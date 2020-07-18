/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <Core.h>
#include <ConsoleUtils.h>
#include <ply-build-folder/BuildFolder.h>
#include <ply-build-repo/ProjectInstantiator.h>
#include <ply-runtime/algorithm/Find.h>
#include <ply-cli/CommandLine.h>

namespace ply {

s32 folder_listHandler(PlyToolCommandEnv* env) {
    using namespace build;

    StringWriter sw = StdOut::createStringWriter();
    sw << "Build folders found:\n";
    for (const BuildFolder* bf : env->buildFolders) {
        PLY_ASSERT(!bf->buildFolderName.isEmpty());
        bool isActive = (bf->buildFolderName == env->workspace->currentBuildFolder);
        sw.format("    {}{}\n", bf->buildFolderName, (isActive ? " (active)" : ""));
    }

    return 0;
}

s32 folder_createHandler(PlyToolCommandEnv* env) {
    using namespace build;

    StringView name;
    auto nameArg = env->context->argument("name");
    if (nameArg) {
        name = nameArg->value();
    }

    if (find(env->buildFolders.view(),
             [&](const BuildFolder* bf) { return bf->buildFolderName == name; }) >= 0) {
        fatalError(String::format("Folder \"{}\" already exists", name));
        return 1;
    }

    Owned<BuildFolder> info = BuildFolder::create(name, name);
    info->cmakeOptions = NativeToolchain;
    if (env->workspace->defaultCMakeOptions.generator) {
        info->cmakeOptions = env->workspace->defaultCMakeOptions;
    }
    info->activeConfig = DefaultNativeConfig;
    if (env->workspace->defaultConfig) {
        info->activeConfig = env->workspace->defaultConfig;
    }
    info->save();
    StdOut::createStringWriter().format("Created build folder '{}' at: {}\n", info->buildFolderName,
                                        info->getAbsPath());
    env->workspace->currentBuildFolder = name;
    env->workspace->save();
    StdOut::createStringWriter().format("'{}' is now the current build folder.\n",
                                        info->buildFolderName);

    return 0;
}

s32 folder_deleteHandler(PlyToolCommandEnv* env) {
    using namespace build;

    StringView name = env->context->argument("name")->value();

    s32 index = find(env->buildFolders.view(),
                     [&](const BuildFolder* bf) { return bf->buildFolderName == name; });
    if (index < 0) {
        fatalError(String::format("Folder \"{}\" does not exist", name));
    }

    // FIXME: Add confirmation prompt or -f/--force option
    BuildFolder* bf = env->buildFolders[index];
    if (FileSystem::native()->removeDirTree(bf->getAbsPath()) == FSResult::OK) {
        StdOut::createStringWriter().format("Deleted build folder '{}'.\n", bf->buildFolderName);
    } else {
        fatalError(String::format("Can't delete build folder '{}'.\n", bf->buildFolderName));
        return 1;
    }

    return 0;
}

s32 folder_setHandler(PlyToolCommandEnv* env) {
    using namespace build;

    StringView name = env->context->argument("name")->value();

    if (find(env->buildFolders.view(),
             [&](const BuildFolder* bf) { return bf->buildFolderName == name; }) < 0) {
        fatalError(String::format("Folder \"{}\" does not exist", name));
        return 1;
    }

    env->workspace->currentBuildFolder = name;
    env->workspace->save();
    StdOut::createStringWriter().format("'{}' is now the current build folder.\n", name);

    return 0;
}

void buildCommand_folder(cli::Command* root, PlyToolCommandEnv* env) {
    root->subCommand(
        "folder", "Manage build folders in the current workspace.", [env](auto& folder) {
            folder.subCommand("list", "List all the build folders in the workspace.",
                              [env](auto& c) { c.handler(wrapHandler(env, folder_listHandler)); });

            folder.subCommand("create", "Create a new build folder in the workspace.",
                              [env](auto& c) {
                                  c.argument("name", "The name of the new build folder.");
                                  c.handler(wrapHandler(env, folder_createHandler));
                              });

            folder.subCommand("delete", "Delete a build folder from the workspace.",
                              [env](auto& c) {
                                  c.argument("name", "The name of the build folder to delete.");
                                  c.handler(wrapHandler(env, folder_deleteHandler));
                              });

            folder.subCommand(
                "set", "Set the active build folder in the workspace.", [env](auto& c) {
                    c.argument("name", "The name of the build folder that will be set to active.");
                    c.handler(wrapHandler(env, folder_setHandler));
                });
        });
}

} // namespace ply
