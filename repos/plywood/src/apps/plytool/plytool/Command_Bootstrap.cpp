/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <Core.h>
#include <ConsoleUtils.h>
#include <ply-build-repo/RepoRegistry.h>
#include <ply-build-repo/ProjectInstantiator.h>
#include <ply-build-provider/ExternFolderRegistry.h>
#include <ply-build-folder/BuildFolder.h>
#include <ply-cli/CommandLine.h>

namespace ply {

s32 bootstrap_handler(PlyToolCommandEnv*) {
    using namespace build;

    PLY_SET_IN_SCOPE(RepoRegistry::instance_, RepoRegistry::create());
    PLY_SET_IN_SCOPE(ExternFolderRegistry::instance_, ExternFolderRegistry::create());

    const TargetInstantiator* target =
        RepoRegistry::get()->findTargetInstantiator("plywood.plytool");
    if (!target) {
        StdErr::createStringWriter() << "Error: Can't find 'plytool' module in 'plywood' repo.\n";
        return 1;
    }

    Owned<BuildFolder> buildFolder = BuildFolder::create({}, "plytool");
    buildFolder->rootTargets.append("plywood.plytool");
    buildFolder->makeShared.append("plywood.plytool");
    ProjectInstantiationResult instResult = buildFolder->instantiateAllTargets(true);
    if (!instResult.isValid) {
        return 1;
    }
    if (instResult.unselectedExterns.numItems() > 0 ||
        instResult.uninstalledProviders.numItems() > 0) {
        StdErr::createStringWriter()
            << "Error: Bootstrap file must not have external dependencies.\n";
        return 1;
    }

    StringWriter sw;
    writeCMakeLists(&sw, buildFolder->solutionName, buildFolder->getAbsPath(), &instResult, true);
    String savePath =
        NativePath::join(PLY_WORKSPACE_FOLDER, "repos/plywood/scripts/bootstrap_CMakeLists.txt");
    FSResult result = FileSystem::native()->makeDirsAndSaveTextIfDifferent(
        savePath, sw.moveToString(), TextFormat::platformPreference());
    if (result == FSResult::OK) {
        StdOut::createStringWriter().format("Successfully wrote bootstrap file '{}'.\n", savePath);
    } else if (result == FSResult::Unchanged) {
        StdOut::createStringWriter().format("Bootstrap file '{}' is already up-to-date.\n",
                                            savePath);
    } else {
        StdErr::createStringWriter().format("Error writing bootstrap file '{}'.\n", savePath);
    }

    return 0;
}

void buildCommand_bootstrap(cli::Command* root, PlyToolCommandEnv* env) {
    root->subCommand(
        "boostrap", "Bootstrap the build files for plytool.",
        [env](auto& c) { c.handler(wrapHandler(env, bootstrap_handler)); });
}

} // namespace ply
