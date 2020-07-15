/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <Core.h>
#include <CommandHelpers.h>
#include <ply-runtime/algorithm/Find.h>
#include <ply-build-folder/BuildFolder.h>
#include <ply-build-repo/RepoRegistry.h>
#include <ply-build-target/BuildTarget.h>
#include <ply-cli/Context.h>

namespace ply {

void AddParams::extractOptions(cli::Context* context) {
    auto sharedOption = context->option("shared");
    if (sharedOption) {
        this->makeShared = sharedOption->isPresent();
    }
}

bool AddParams::exec(build::BuildFolder* folder, StringView fullTargetName) {
    bool anyChange = false;
    if (findItem(folder->rootTargets.view(), fullTargetName) < 0) {
        folder->rootTargets.append(fullTargetName);
        anyChange = true;
    }
    if (this->makeShared && findItem(folder->makeShared.view(), fullTargetName) < 0) {
        folder->makeShared.append(fullTargetName);
        anyChange = true;
    }
    if (folder->activeTarget != fullTargetName) {
        folder->activeTarget = fullTargetName;
        anyChange = true;
    }
    return anyChange;
}

// static
void BuildParams::addCommandLineOptions(cli::Command* command) {
    // TODO: Add descriptions for these options.
    command->add(cli::Option{"config", ""});
    command->add(cli::Option{"add", ""});
    command->add(cli::Option{"auto", ""});
    command->add(cli::Option{"shared", ""});
    command->add(cli::Argument{"target", "The target to build"});
}

void BuildParams::extractOptions(cli::Context* context) {
    auto configOption = context->option("config");
    if (configOption) {
        this->configName = configOption->value();
    }

    auto doAddOption = context->option("add");
    if (doAddOption) {
        this->doAdd = doAddOption->isPresent();
    }

    auto autoOption = context->option("auto");
    if (autoOption) {
        this->doAuto = autoOption->isPresent();
    }

    if (this->doAdd || this->doAuto) {
        this->addParams.extractOptions(context);
    }

    auto targetArg = context->argument("target");
    if (targetArg) {
        this->targetName = targetArg->value();
    }
}

bool BuildParams::exec(BuildParams::Result* result, PlyToolCommandEnv* env, bool doBuild) {
    using namespace build;

    // Chooes a build folder
    result->folder = env->currentBuildFolder;
    if (this->doAuto) {
        if (!this->targetName) {
            fatalError("A target name must be specified when --auto is used");
        }
        const TargetInstantiator* buildTargetInst =
            RepoRegistry::get()->findTargetInstantiator(this->targetName);
        if (!buildTargetInst) {
            exit(1); // Error was already logged
        }

        // Find build folders containing this target
        Array<Tuple<BuildFolder*, const TargetInstantiator*>> matches;
        for (BuildFolder* bf : env->buildFolders) {
            for (StringView curTargetName : bf->rootTargets) {
                const TargetInstantiator* curTargetInst =
                    RepoRegistry::get()->findTargetInstantiator(curTargetName);
                if (curTargetInst == buildTargetInst) {
                    matches.append({bf, curTargetInst});
                    break;
                }
            }
        }

        // Handle the number of matches
        if (matches.numItems() == 0) {
            // Create a new build folder
            Owned<BuildFolder> newFolder = BuildFolder::autocreate(this->targetName);
            newFolder->cmakeOptions = NativeToolchain;
            if (env->workspace->defaultCMakeOptions.generator) {
                newFolder->cmakeOptions = env->workspace->defaultCMakeOptions;
            }
            newFolder->activeConfig = DefaultNativeConfig;
            if (env->workspace->defaultConfig) {
                newFolder->activeConfig = env->workspace->defaultConfig;
            }
            this->addParams.exec(newFolder, buildTargetInst->getFullyQualifiedName());
            newFolder->save();
            StdOut::createStringWriter().format(
                "Created build folder '{}' with root target '{}' at: {}\n",
                newFolder->buildFolderName,
                RepoRegistry::get()->getShortDepSourceName(buildTargetInst),
                newFolder->getAbsPath());
            result->folder = newFolder;
            env->buildFolders.append(std::move(newFolder));
            if (env->workspace->currentBuildFolder != result->folder->buildFolderName) {
                env->workspace->currentBuildFolder = result->folder->buildFolderName;
                env->workspace->save();
                StdOut::createStringWriter().format("'{}' is now the current build folder.\n",
                                                    result->folder->buildFolderName);
            }
        } else if (matches.numItems() == 1) {
            result->folder = matches[0].first;
            if (env->workspace->currentBuildFolder != result->folder->buildFolderName) {
                env->workspace->currentBuildFolder = result->folder->buildFolderName;
                env->workspace->save();
                StdOut::createStringWriter().format("'{}' is now the current build folder.\n",
                                                    result->folder->buildFolderName);
            }
        } else {
            StringWriter sw;
            sw.format("Can't use --auto because target '{}' was found in {} build folders: ",
                      this->targetName, matches.numItems());
            for (u32 i = 0; i < matches.numItems(); i++) {
                sw.format("'{}'", matches[i].first->buildFolderName);
                if (i + 2 < matches.numItems()) {
                    sw << ", ";
                } else if (i + 2 == matches.numItems()) {
                    sw << " and ";
                }
            }
            fatalError(sw.moveToString());
        }
    } else {
        if (!result->folder) {
            fatalError("Current build folder not set");
        }
    }

    // FIXME: Factor this out into a common function with command_generate so that the reason for
    // instantiation failure is logged.
    result->instResult = result->folder->instantiateAllTargets(false);
    if (!result->instResult.isValid) {
        fatalError(String::format("Can't instantiate dependencies for folder '{}'\n",
                                  result->folder->buildFolderName));
    }

    if (!this->targetName) {
        this->targetName = result->folder->activeTarget;
    }

    result->runTargetInst = RepoRegistry::get()->findTargetInstantiator(this->targetName);
    if (!result->runTargetInst) {
        exit(1);
    }

    auto runTargetIdx = result->instResult.dependencyMap.find(result->runTargetInst,
                                                              &result->instResult.dependencies);
    if (!runTargetIdx.wasFound()) {
        if (this->doAdd) {
            this->addParams.exec(result->folder, result->runTargetInst->getFullyQualifiedName());
            StdOut::createStringWriter().format(
                "Added target '{}' to folder '{}'.\n",
                RepoRegistry::get()->getShortDepSourceName(result->runTargetInst),
                result->folder->buildFolderName);
            result->folder->save();

            // Re-instantiate all targets
            result->instResult = result->folder->instantiateAllTargets(false);
            if (!result->instResult.isValid) {
                fatalError(String::format("Can't instantiate dependencies for folder '{}'\n",
                                          result->folder->buildFolderName));
            }
            runTargetIdx = result->instResult.dependencyMap.find(result->runTargetInst,
                                                                 &result->instResult.dependencies);
            PLY_ASSERT(runTargetIdx.wasFound());
        } else {
            fatalError(
                String::format("Target '{}' is not instantiated in folder '{}'\n",
                               RepoRegistry::get()->getShortDepSourceName(result->runTargetInst),
                               result->folder->buildFolderName));
        }
    }

    result->runTarget =
        static_cast<BuildTarget*>(result->instResult.dependencies[*runTargetIdx].dep.get());
    PLY_ASSERT(result->runTarget->type == DependencyType::Target);
    if (result->runTarget->targetType != BuildTargetType::EXE) {
        fatalError(String::format("Target '{}' is not executable\n", this->targetName));
    }

    // Generate & build
    if (!this->configName) {
        this->configName = result->folder->activeConfig;
    }
    if (doBuild) {
        if (!result->folder->isGenerated(this->configName)) {
            if (!result->folder->generateLoop(this->configName))
                return false;
        }
        if (!result->folder->build(this->configName, result->runTarget->name, false))
            return false;
    }

    return true;
}

} // namespace ply
