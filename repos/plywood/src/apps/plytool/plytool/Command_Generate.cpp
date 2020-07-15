/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <Core.h>
#include <ConsoleUtils.h>
#include <ply-build-folder/BuildFolder.h>
#include <ply-build-repo/RepoRegistry.h>
#include <ply-build-provider/ExternFolderRegistry.h>
#include <ply-build-provider/HostTools.h>
#include <ply-cli/CommandLine.h>

namespace ply {

s32 generate_handler(PlyToolCommandEnv* env) {
    using namespace build;
    if (!env->currentBuildFolder) {
        fatalError("Current build folder not set");
    }

    StringView configName;
    auto configOption = env->context->option("config");
    if (configOption) {
        configName = configOption->value();
    }

    PLY_SET_IN_SCOPE(RepoRegistry::instance_, RepoRegistry::create());
    PLY_SET_IN_SCOPE(ExternFolderRegistry::instance_, ExternFolderRegistry::create());
    PLY_SET_IN_SCOPE(HostTools::instance_, HostTools::create());

    return env->currentBuildFolder->generateLoop(configName) ? 0 : 1;
}

void buildCommand_generate(cli::Command* root, PlyToolCommandEnv* env) {
    cli::Command cmd{"generate", "Generate the build environment for the current build folder"};
    cmd.add(cli::Option{"config", "The configuration to use"});
    cmd.handler(wrapHandler(env, generate_handler));

    root->add(std::move(cmd));
}

} // namespace ply
