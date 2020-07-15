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
#include <ply-build-target/BuildTarget.h>
#include <ply-cli/CommandLine.h>

namespace ply {

s32 run_handler(PlyToolCommandEnv* env) {
    using namespace build;

    BuildParams buildParams;
    buildParams.extractOptions(env->context);

    bool doBuild = env->context->option("nobuild")->isPresent();

    PLY_SET_IN_SCOPE(RepoRegistry::instance_, RepoRegistry::create());
    PLY_SET_IN_SCOPE(ExternFolderRegistry::instance_, ExternFolderRegistry::create());
    PLY_SET_IN_SCOPE(HostTools::instance_, HostTools::create());

    BuildParams::Result buildResult;
    if (!buildParams.exec(&buildResult, env, doBuild))
        return -1;

    String exePath =
        getTargetOutputPath(buildResult.runTarget->targetType, buildResult.runTarget->name,
                            buildResult.folder->getAbsPath(), buildParams.configName);
    if (FileSystem::native()->exists(exePath) != ExistsResult::File) {
        fatalError(
            String::format("Executable '{}' is not built in folder '{}'\n",
                           RepoRegistry::get()->getShortDepSourceName(buildResult.runTargetInst),
                           buildResult.folder->buildFolderName));
    }

    // FIXME: Implement --args option (should be last option on command line; rest of command line
    // consists of arguments to pass
    StdOut::createStringWriter().format("Running '{}'...\n", exePath);
    Owned<Subprocess> child = Subprocess::exec(exePath, {}, {}, Subprocess::Output::inherit());
    return child->join();
}

void buildCommand_run(cli::Command* root, PlyToolCommandEnv* env) {
    cli::Command cmd{"run", "Run the executable build target of the current build folder."};
    cmd.add(cli::Option{"nobuild", "Do not build before running the target"});
    BuildParams::addCommandLineOptions(&cmd);
    cmd.handler(wrapHandler(env, run_handler));

    root->add(std::move(cmd));
}

} // namespace ply
