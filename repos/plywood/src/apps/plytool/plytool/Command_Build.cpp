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
#include <ply-cli/CommandLine.h>

namespace ply {

s32 build_handler(PlyToolCommandEnv* env) {
    using namespace build;

    BuildParams buildParams;
    buildParams.extractOptions(env->context);

    PLY_SET_IN_SCOPE(RepoRegistry::instance_, RepoRegistry::create());
    PLY_SET_IN_SCOPE(ExternFolderRegistry::instance_, ExternFolderRegistry::create());
    PLY_SET_IN_SCOPE(HostTools::instance_, HostTools::create());

    BuildParams::Result buildResult;
    return buildParams.exec(&buildResult, env, true) ? 0 : 1;
}

void buildCommand_build(cli::Command* root, PlyToolCommandEnv* env) {
    root->subCommand("build", "Build the specified target in the current build folder.",
                     [env](auto& c) {
                         BuildParams::addCommandLineOptions(&c);
                         c.handler(wrapHandler(env, build_handler));
                     });
}

} // namespace ply
