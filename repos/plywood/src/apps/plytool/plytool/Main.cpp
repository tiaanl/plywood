/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <Core.h>
#include <ConsoleUtils.h>
#include <ply-build-folder/BuildFolder.h>
#include <ply-build-repo/ProjectInstantiator.h>
#include <ply-runtime/algorithm/Find.h>
#include <ply-build-repo/ErrorHandler.h>

namespace ply {

void buildCommand_rpc(cli::Command* root, PlyToolCommandEnv* env);
void buildCommand_cleanup(cli::Command* root, PlyToolCommandEnv* env);
void buildCommand_open(cli::Command* root, PlyToolCommandEnv* env);
void buildCommand_bootstrap(cli::Command* root, PlyToolCommandEnv* env);
void buildCommand_run(cli::Command* root, PlyToolCommandEnv* env);
void buildCommand_build(cli::Command* root, PlyToolCommandEnv* env);
void buildCommand_generate(cli::Command* root, PlyToolCommandEnv* env);
void buildCommand_codegen(cli::Command* root, PlyToolCommandEnv* env);
void buildCommand_folder(cli::Command* root, PlyToolCommandEnv* env);
void buildCommand_extern(cli::Command* root, PlyToolCommandEnv* env);
void buildCommand_module(cli::Command* root, PlyToolCommandEnv* env);
void buildCommand_target(cli::Command* root, PlyToolCommandEnv* env);

} // namespace ply

int main(int argc, char* argv[]) {
    using namespace ply;

    auto errorHandler = [](build::ErrorHandler::Level errorLevel, HybridString&& error) {
        StringWriter sw;
        if (errorLevel == build::ErrorHandler::Error || errorLevel == build::ErrorHandler::Fatal) {
            sw = StdErr::createStringWriter();
            sw << "Error: ";
        } else {
            sw = StdOut::createStringWriter();
        }
        sw << error.view();
        if (!error.view().endsWith("\n")) {
            sw << '\n';
        }
    };
    PLY_SET_IN_SCOPE(build::ErrorHandler::current, errorHandler);

    WorkspaceSettings workspace;
    if (!workspace.load()) {
        fatalError(String::format("Invalid workspace settings file: \"{}\"",
                                  WorkspaceSettings::getPath()));
    }

    CommandLine cl{argc, argv};

    PlyToolCommandEnv env;
    env.cl = &cl;
    env.workspace = &workspace;

    cli::CommandLine commandLine{"plytool", "Plytool manages a Plywood workspace."};
    buildCommand_rpc(&commandLine, &env);
    buildCommand_cleanup(&commandLine, &env);
    buildCommand_bootstrap(&commandLine, &env);
    buildCommand_open(&commandLine, &env);
    buildCommand_build(&commandLine, &env);
    buildCommand_run(&commandLine, &env);
    buildCommand_generate(&commandLine, &env);
    buildCommand_folder(&commandLine, &env);
    buildCommand_extern(&commandLine, &env);
    buildCommand_module(&commandLine, &env);
    buildCommand_target(&commandLine, &env);
    auto context = commandLine.parse(argc, argv);

    // FIXME: Only call getBuildFolders when really needed
    env.buildFolders = build::BuildFolder::getList();
    s32 defaultIndex = find(env.buildFolders.view(), [&](const build::BuildFolder* bf) {
        return bf->buildFolderName == workspace.currentBuildFolder;
    });
    if (defaultIndex >= 0) {
        env.currentBuildFolder = env.buildFolders[defaultIndex];
    }

    auto sw = StdOut::createStringWriter();
    return context.run(&sw);
    // context.printUsage(&sw);
    // return 0;

#if 0
    StringView category = cl.readToken();
    bool success = true;
    if (category.isEmpty()) {
        cl.finalize();
        auto sw = StdErr::createStringWriter();
        const CommandList commands = {
            {"bootstrap", "bootstrap description"}, {"build", "build description"},
            {"cleanup", "cleanup description"},     {"codegen", "codegen description"},
            {"extern", "extern description"},       {"folder", "folder description"},
            {"generate", "generate description"},   {"module", "module description"},
            {"target", "target description"},
        };
        printUsage(&sw, commands);
    } else if (category == "rpc") {
        command_rpc(&env);
    } else if (prefixMatch(category, "folder")) {
        command_folder(&env);
    } else if (prefixMatch(category, "target")) {
        command_target(&env);
    } else if (prefixMatch(category, "module")) {
        command_module(&env);
    } else if (prefixMatch(category, "extern")) {
        command_extern(&env);
    } else if (prefixMatch(category, "generate")) {
        success = command_generate(&env);
    } else if (prefixMatch(category, "build")) {
        success = command_build(&env);
    } else if (prefixMatch(category, "run")) {
        return command_run(&env);
    } else if (prefixMatch(category, "open")) {
        success = command_open(&env);
    } else if (prefixMatch(category, "codegen")) {
        command_codegen(&env);
    } else if (prefixMatch(category, "bootstrap")) {
        command_bootstrap(&env);
    } else if (prefixMatch(category, "cleanup")) {
        command_cleanup(&env);
    } else {
        fatalError(String::format("Unrecognized command \"{}\"", category));
    }

    return success ? 0 : 1;
#endif // 0
}
