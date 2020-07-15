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
    cli::Command listCommand{"list", "List all the build folders in the workspace."};
    listCommand.handler(wrapHandler(env, folder_listHandler));

    cli::Command createCommand{"create", "Create a new build folder in the workspace."};
    createCommand.add(cli::Argument{"name", "The name of the new build folder."});
    createCommand.handler(wrapHandler(env, folder_createHandler));

    cli::Command deleteCommand{"delete", "Delete a build folder from the workspace."};
    deleteCommand.add(cli::Argument{"name", "The name of the build folder to delete."});
    deleteCommand.handler(wrapHandler(env, folder_deleteHandler));

    cli::Command setCommand{"set", "Set the active build folder in the workspace."};
    setCommand.add(
        cli::Argument{"name", "The name of the build folder that will be set to active."});
    setCommand.handler(wrapHandler(env, folder_setHandler));

    cli::Command cmd{"folder", "Manage build folders in the current workspace."};
    cmd.add(std::move(listCommand));
    cmd.add(std::move(createCommand));
    cmd.add(std::move(deleteCommand));
    cmd.add(std::move(setCommand));

    root->add(std::move(cmd));
}

} // namespace ply
