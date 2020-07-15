/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <Core.h>
#include <ConsoleUtils.h>
#include <ply-build-folder/BuildFolder.h>
#include <ply-build-repo/RepoRegistry.h>
#include <ply-build-repo/ProjectInstantiator.h>
#include <ply-build-provider/HostTools.h>
#include <ply-build-provider/ExternFolderRegistry.h>

namespace ply {

void installProvider(PlyToolCommandEnv* env, const build::ExternProvider* provider) {
    using namespace build;
    PLY_SET_IN_SCOPE(ExternFolderRegistry::instance_, ExternFolderRegistry::create());
    PLY_SET_IN_SCOPE(HostTools::instance_, HostTools::create());

    StdOut::createStringWriter().format("Installing extern provider '{}'...\n",
                                        RepoRegistry::get()->getShortProviderName(provider));

    Owned<pylon::Node> toolchain =
        toolchainInfoFromCMakeOptions(env->currentBuildFolder->cmakeOptions);
    ExternProviderArgs args;
    args.toolchain = toolchain;
    args.provider = provider;
    ExternResult er = provider->externFunc(ExternCommand::Install, &args);
    String errorDetails;
    if (er.details) {
        errorDetails = String::format(" ({})", er.details);
    }
    switch (er.code) {
        case ExternResult::BadArgs: {
            fatalError(String::format("bad provider arguments{}\n", errorDetails));
            break;
        }
        case ExternResult::UnsupportedHost: {
            fatalError(String::format("can't install on host{}\n", errorDetails));
            break;
        }
        case ExternResult::MissingPackageManager: {
            fatalError(String::format("missing package manager{}\n", errorDetails));
            break;
        }
        case ExternResult::UnsupportedToolchain: {
            fatalError(String::format("toolchain is not supported{}\n", errorDetails));
            break;
        }
        case ExternResult::Installed: {
            StdOut::createStringWriter()
                << String::format("Successfully installed extern provider '{}'.\n",
                                  RepoRegistry::get()->getShortProviderName(provider));
            break;
        }
        case ExternResult::InstallFailed: {
            fatalError(String::format("install failed{}\n", errorDetails));
            break;
        }
        default: {
            fatalError("internal provider returned invalid code");
            break;
        }
    }
}

s32 extern_externHandler(PlyToolCommandEnv* env) {
    using namespace build;
    return 0;
}

s32 extern_listHandler(PlyToolCommandEnv* env) {
    using namespace build;

    if (!env->currentBuildFolder) {
        fatalError("Current build folder not set");
        return 1;
    }

    PLY_SET_IN_SCOPE(RepoRegistry::instance_, RepoRegistry::create());

    for (const Repo* repo : RepoRegistry::get()->repos) {
        StringWriter sw = StdOut::createStringWriter();
        u32 n = repo->externs.numItems();
        sw.format("{} extern{} defined in repo '{}'{}\n", n, n == 1 ? "" : "s", repo->repoName,
                  n == 0 ? "." : ":");
        for (const DependencySource* extern_ : repo->externs) {
            sw.format("    {}\n", RepoRegistry::get()->getShortDepSourceName(extern_));
        }
    }

    return 0;
}

s32 extern_infoHandler(PlyToolCommandEnv* env) {
    using namespace build;

    if (!env->currentBuildFolder) {
        fatalError("Current build folder not set");
        return 1;
    }

    StringView externName = env->context->argument("name")->value();

    PLY_SET_IN_SCOPE(RepoRegistry::instance_, RepoRegistry::create());
    PLY_SET_IN_SCOPE(ExternFolderRegistry::instance_, ExternFolderRegistry::create());
    PLY_SET_IN_SCOPE(HostTools::instance_, HostTools::create());

    const DependencySource* extern_ = RepoRegistry::get()->findExtern(externName);
    if (!extern_) {
        fatalError(String::format("Can't find extern '{}'", externName));
    }

    Array<Tuple<const ExternProvider*, ExternResult::Code>> candidates;
    for (const Repo* repo : RepoRegistry::get()->repos) {
        for (const ExternProvider* externProvider : repo->externProviders) {
            if (externProvider->extern_ != extern_)
                continue;
            Owned<pylon::Node> toolchain =
                toolchainInfoFromCMakeOptions(env->currentBuildFolder->cmakeOptions);
            ExternProviderArgs args;
            args.toolchain = toolchain;
            args.provider = externProvider;
            ExternResult er = externProvider->externFunc(ExternCommand::Status, &args);
            candidates.append({externProvider, er.code});
        }
    }
    u32 n = candidates.numItems();
    StringWriter sw = StdOut::createStringWriter();
    sw.format("Found {} provider{} for extern '{}':\n", n, n == 1 ? "" : "s",
              RepoRegistry::get()->getShortDepSourceName(extern_));
    for (Tuple<const ExternProvider*, ExternResult::Code> pair : candidates) {
        StringView codeStr = "???";
        switch (pair.second) {
            case ExternResult::Installed: {
                codeStr = "installed";
                break;
            }
            case ExternResult::SupportedButNotInstalled: {
                codeStr = "not installed";
                break;
            }
            case ExternResult::UnsupportedHost:
            case ExternResult::MissingPackageManager: {
                codeStr = "not supported on host system";
                break;
            }
            case ExternResult::UnsupportedToolchain: {
                codeStr = "not compatible with build folder";
                break;
            }
            default:
                break;
        }
        sw.format("    {} ({})\n", RepoRegistry::get()->getShortProviderName(pair.first), codeStr);
    }

    return 0;
}

s32 extern_selectHandler(PlyToolCommandEnv* env) {
    using namespace build;

    if (!env->currentBuildFolder) {
        fatalError("Current build folder not set");
        return 1;
    }

    StringView qualifiedName = env->context->argument("name")->value();
    bool shouldInstall = env->context->option("install")->isPresent();

    PLY_SET_IN_SCOPE(RepoRegistry::instance_, RepoRegistry::create());
    const ExternProvider* provider = RepoRegistry::get()->getExternProvider(qualifiedName);
    if (!provider) {
        fatalError("Provider not found");
    }

    if (shouldInstall) {
        installProvider(env, provider);
    }

    // Remove any selected providers that match the same extern
    for (u32 i = 0; i < env->currentBuildFolder->externSelectors.numItems();) {
        const ExternProvider* other =
            RepoRegistry::get()->getExternProvider(env->currentBuildFolder->externSelectors[i]);
        if (other && other->extern_ == provider->extern_) {
            env->currentBuildFolder->externSelectors.erase(i);
        } else {
            i++;
        }
    }

    env->currentBuildFolder->externSelectors.append(provider->getFullyQualifiedName());
    env->currentBuildFolder->save();

    StdOut::createStringWriter().format("Selected extern provider '{}' in build folder '{}'.\n",
                                        RepoRegistry::get()->getShortProviderName(provider),
                                        env->currentBuildFolder->buildFolderName);

    return 0;
}

s32 extern_selectedHandler(PlyToolCommandEnv* env) {
    using namespace build;

    if (!env->currentBuildFolder) {
        fatalError("Current build folder not set");
        return 1;
    }

    StringWriter sw = StdOut::createStringWriter();
    sw.format("The following extern providers are selected in build folder '{}':\n",
              env->currentBuildFolder->buildFolderName);
    for (StringView sel : env->currentBuildFolder->externSelectors) {
        sw.format("    {}\n", sel);
    }

    return 0;
}

s32 extern_installHandler(PlyToolCommandEnv* env) {
    using namespace build;

    if (!env->currentBuildFolder) {
        fatalError("Current build folder not set");
        return 1;
    }

    StringView qualifiedName = env->context->argument("name")->value();

    PLY_SET_IN_SCOPE(RepoRegistry::instance_, RepoRegistry::create());
    const ExternProvider* provider = RepoRegistry::get()->getExternProvider(qualifiedName);
    if (!provider) {
        fatalError("Provider not found");
        return 1;
    }

    installProvider(env, provider);

    return 0;
}

void buildCommand_extern(cli::Command* root, PlyToolCommandEnv* env) {
    cli::Command listCmd{"list", "List all the available external dependencies."};
    listCmd.handler(wrapHandler(env, extern_listHandler));

    cli::Command infoCmd{"info", "Show info about the specified external dependency."};
    infoCmd.add(
        cli::Argument{"name", "The name of the external dependency to show information for."});
    infoCmd.handler(wrapHandler(env, extern_infoHandler));

    cli::Command selectCmd{"select",
                           "Select an external dependency to use in the current build folder."};
    selectCmd.add(
        cli::Argument{"install", "Specify that the extern should be install if it hasn't yet."});
    selectCmd.add(
        cli::Argument{"name", "The fully qualified name of the external dependency to select."});
    selectCmd.handler(wrapHandler(env, extern_selectHandler));

    cli::Command selectedCmd(
        "selected", "Show the currently selected target for the specified external dependency.");
    selectedCmd.handler(wrapHandler(env, extern_selectedHandler));

    cli::Command installCmd("install", "Install the selected external dependency.");
    infoCmd.add(
        cli::Argument{"name", "The fully qualified name of the external dependency to install."});
    installCmd.handler(wrapHandler(env, extern_installHandler));

    cli::Command cmd{"extern", "Manage external dependencies for the current build folder."};
    cmd.add(std::move(listCmd));
    cmd.add(std::move(infoCmd));
    cmd.add(std::move(selectCmd));
    cmd.add(std::move(selectedCmd));
    cmd.add(std::move(installCmd));

    root->add(std::move(cmd));
}

} // namespace ply
