/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <Core.h>
#include <ConsoleUtils.h>
#include <CommandHelpers.h>
#include <ply-build-folder/BuildFolder.h>
#include <ply-build-repo/RepoRegistry.h>
#include <ply-build-provider/ExternFolderRegistry.h>
#include <ply-build-provider/HostTools.h>
#include <ply-runtime/algorithm/Find.h>
#include <ply-cli/CommandLine.h>

namespace ply {

struct DepTreeIndent {
    String node;
    String children;
};

PLY_NO_INLINE void dumpDepTree(StringWriter* sw, const build::DependencyTree* depTreeNode,
                               const DepTreeIndent& indent) {
    sw->format("{}{}\n", indent.node, depTreeNode->desc);
    for (u32 i = 0; i < depTreeNode->children.numItems(); i++) {
        DepTreeIndent childIndent;
        if (i + 1 < depTreeNode->children.numItems()) {
            childIndent.node = indent.children + "+-- ";
            childIndent.children = indent.children + "|   ";
        } else {
            childIndent.node = indent.children + "`-- ";
            childIndent.children = indent.children + "    ";
        }
        dumpDepTree(sw, &depTreeNode->children[i], childIndent);
    }
}

s32 target_listHandler(PlyToolCommandEnv* env) {
    using namespace build;

    if (!env->currentBuildFolder) {
        fatalError("Current build folder not set");
    }
    PLY_SET_IN_SCOPE(RepoRegistry::instance_, RepoRegistry::create());

    StringWriter sw = StdOut::createStringWriter();
    sw.format("List of root targets in build folder '{}':\n",
              env->currentBuildFolder->buildFolderName);
    for (StringView targetName : env->currentBuildFolder->rootTargets) {
        const TargetInstantiator* targetInst =
            RepoRegistry::get()->findTargetInstantiator(targetName);
        if (!targetInst) {
            sw.format("    {} (not found)\n", targetName);
        } else {
            String activeText;
            if (targetName == env->currentBuildFolder->activeTarget) {
                activeText = " (active)";
            }
            sw.format("    {}{}\n", RepoRegistry::get()->getShortDepSourceName(targetInst),
                      activeText);
        }
    }

    return 0;
}

s32 target_addHandler(PlyToolCommandEnv* env) {
    using namespace build;

    if (!env->currentBuildFolder) {
        fatalError("Current build folder not set");
    }
    PLY_SET_IN_SCOPE(RepoRegistry::instance_, RepoRegistry::create());

    StringView targetName = env->context->argument("name")->value();

    AddParams addParams;
    // addParams.extractOptions(context);

    const TargetInstantiator* targetInst = RepoRegistry::get()->findTargetInstantiator(targetName);
    if (!targetInst) {
        fatalError(String::format("Can't find target '{}'", targetName));
    }
    String fullTargetName = targetInst->getFullyQualifiedName();

    addParams.exec(env->currentBuildFolder, fullTargetName);

    env->currentBuildFolder->save();
    StdOut::createStringWriter().format("Added root target '{}' to build folder '{}'.\n",
                                        RepoRegistry::get()->getShortDepSourceName(targetInst),
                                        env->currentBuildFolder->buildFolderName);

    return 0;
}

s32 target_removeHandler(PlyToolCommandEnv* env) {
    using namespace build;

    if (!env->currentBuildFolder) {
        fatalError("Current build folder not set");
    }
    PLY_SET_IN_SCOPE(RepoRegistry::instance_, RepoRegistry::create());

    StringView targetName = env->context->argument("name")->value();

    const TargetInstantiator* targetInst = RepoRegistry::get()->findTargetInstantiator(targetName);
    if (!targetInst) {
        exit(1);
    }

    String fullTargetName = targetInst->getFullyQualifiedName();
    s32 j = findItem(env->currentBuildFolder->rootTargets.view(), fullTargetName);
    if (j < 0) {
        fatalError(String::format("Folder '{}' does not have root target '{}'",
                                  env->currentBuildFolder->buildFolderName,
                                  RepoRegistry::get()->getShortDepSourceName(targetInst)));
    }
    env->currentBuildFolder->rootTargets.erase(j);
    env->currentBuildFolder->save();
    StdOut::createStringWriter().format("Removed root target '{}' from build folder '{}'.\n",
                                        RepoRegistry::get()->getShortDepSourceName(targetInst),
                                        env->currentBuildFolder->buildFolderName);

    return 0;
}

s32 target_setHandler(PlyToolCommandEnv* env) {
    using namespace build;

    if (!env->currentBuildFolder) {
        fatalError("Current build folder not set");
    }
    PLY_SET_IN_SCOPE(RepoRegistry::instance_, RepoRegistry::create());

    StringView targetName = env->context->argument("name")->value();

    // FIXME: Factor this out into a common function with command_generate so that the reason
    // for instantiation failure is logged.
    PLY_SET_IN_SCOPE(ExternFolderRegistry::instance_, ExternFolderRegistry::create());
    PLY_SET_IN_SCOPE(HostTools::instance_, HostTools::create());
    ProjectInstantiationResult instResult = env->currentBuildFolder->instantiateAllTargets(false);
    if (!instResult.isValid) {
        fatalError(String::format("Can't instantiate dependencies for folder '{}'\n",
                                  env->currentBuildFolder->buildFolderName));
    }

    const TargetInstantiator* targetInst = RepoRegistry::get()->findTargetInstantiator(targetName);
    if (!targetInst) {
        exit(1);
    }

    auto targetIdx = instResult.dependencyMap.find(targetInst, &instResult.dependencies);
    if (!targetIdx.wasFound()) {
        fatalError(String::format("Target '{}' is not instantiated in folder '{}'\n", targetName,
                                  env->currentBuildFolder->buildFolderName));
    }

    env->currentBuildFolder->activeTarget = targetInst->getFullyQualifiedName();
    env->currentBuildFolder->save();
    StdOut::createStringWriter().format("Active target is now '{}' in folder '{}'.\n",
                                        RepoRegistry::get()->getShortDepSourceName(targetInst),
                                        env->currentBuildFolder->buildFolderName);

    return 0;
}

s32 target_graphHandler(PlyToolCommandEnv* env) {
    using namespace build;

    if (!env->currentBuildFolder) {
        fatalError("Current build folder not set");
    }
    PLY_SET_IN_SCOPE(RepoRegistry::instance_, RepoRegistry::create());

    PLY_SET_IN_SCOPE(ExternFolderRegistry::instance_, ExternFolderRegistry::create());
    PLY_SET_IN_SCOPE(HostTools::instance_, HostTools::create());
    DependencyTree depTree = env->currentBuildFolder->buildDepTree();
    StringWriter sw = StdOut::createStringWriter();
    sw.format("Dependency graph for folder '{}':\n", env->currentBuildFolder->buildFolderName);
    DepTreeIndent indent;
    indent.node = "    ";
    indent.children = "    ";
    for (const DependencyTree& treeNode : depTree.children) {
        dumpDepTree(&sw, &treeNode, indent);
    }

    return 0;
}

void buildCommand_target(cli::Command* root, PlyToolCommandEnv* env) {
    cli::Command listCmd{"list", "List targets in the current build folder."};
    listCmd.handler(wrapHandler(env, target_listHandler));

    cli::Command addCmd{"add", "Add a target to the current build folder."};
    addCmd.add(cli::Argument{"name", "Name of the target to add to the current build folder."});
    addCmd.handler(wrapHandler(env, target_addHandler));

    cli::Command removeCmd{"remove", "Remove a target from the current build folder."};
    removeCmd.add(
        cli::Argument("name", "Name of the target to remove from the current build folder."));
    removeCmd.handler(wrapHandler(env, target_removeHandler));

    cli::Command setCmd{"set", "Set the current target in the current build folder."};
    setCmd.add(
        cli::Argument{"name", "Name of the target to set as active in the current build folder."});
    setCmd.handler(wrapHandler(env, target_setHandler));

    cli::Command graphCmd{"graph", "Show a graph of all the targets in the current build folder."};
    graphCmd.handler(wrapHandler(env, target_graphHandler));

    cli::Command cmd{"target", "Manage targets in the selected build folder."};
    cmd.add(std::move(listCmd));
    cmd.add(std::move(addCmd));
    cmd.add(std::move(removeCmd));
    cmd.add(std::move(setCmd));
    cmd.add(std::move(graphCmd));

    root->add(std::move(cmd));
}

} // namespace ply
