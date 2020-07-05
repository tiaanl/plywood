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
#include <ply-cli/CommandLine.h>

namespace ply {

void command_rpc(PlyToolCommandEnv* env);
void command_folder(PlyToolCommandEnv* env);
void command_target(PlyToolCommandEnv* env);
void command_module(PlyToolCommandEnv* env);
void command_extern(PlyToolCommandEnv* env);
bool command_generate(PlyToolCommandEnv* env);
bool command_build(PlyToolCommandEnv* env);
void command_codegen(PlyToolCommandEnv* env);
void command_bootstrap(PlyToolCommandEnv* env);
void command_cleanup(PlyToolCommandEnv* env);
s32 command_run(PlyToolCommandEnv* env);
bool command_open(PlyToolCommandEnv* env);

} // namespace ply

int main(int argc, char* argv[]) {
    using namespace ply;

#if 0
    auto cl = commandLine("plytool", "Manage plywood workspaces", [](auto* c) {
        c->flag("debug", "Enable debug output", [](auto* f) {
            f->shortName("d");
        });

        c->command("bootstrap", "Generate the build environment to bootstrap plytool");

        c->command("build", "Build the targets in the active folder");

        c->command("cleanup");

        c->command("codegen", "Run the codegen command on the active folder");

        c->group("extern", "Manage external libraries for the active folder", [](auto* c) {
            c->command("list");
            c->command("info");
            c->command("select");
            c->command("selected");
            c->command("install");
        });

        c->group("folder", "Manages the active folder in the workspace", [](auto* c) {
            c->flag("root", "Set the root of the plytool workspace", [](auto* f) {
                f->shortName("r");
                f->defaultValue("/");
            });

            c->command("list", "List available folders in the workspace");
            c->command("create", "Create a new folder in the workspace");
            c->command("delete", "Delete a folder in the workspace");
            c->command("set", "Set the active folder in the workspace");
        });

        c->command("generate", "Generate the build environment for the active folder");

        c->group("module", [](auto* c) { c->command("list"); });

        c->group("target", "Manage build targets in the active folder", [](auto* c) {
            c->command("list");
            c->command("add");
            c->command("remove");
            c->command("graph");
        });
    });

    const char* args[] = {
        "plytool",
        "bootstrap",
        "-debug",
        // "folder",
        // "list",
        "-sort",
    };

    auto sw = StdOut::createStringWriter();
    auto context = cl.parse(sizeof(args) / sizeof(const char*), (char**) args);
    if (!context.isRunnable()) {
        context.printUsage(&sw);
    } else {
        context.run();
    }

    // sw << "Command found: " << context.lastCommand->name() << '\n';

    // for (auto& command : context.commands) {
    //     sw << command << '\n';
    // }

    return 0;
#endif // 0

#if 1
    auto sw = StdOut::createStringWriter();

    cli::Command folderListCommand{"list", "List folders in the workspace"};
    folderListCommand.add(
        cli::Option{"verbose", "Enable verbosity in logging"}.shortName("v").defaultValue("4"));
    folderListCommand.handler([&sw](auto* ctx) {
        auto verbosityLevel = ctx->option("verbose")->to<u32>();
        sw << "Running list command with verbosity level: " << verbosityLevel << "\n";

        if (ctx->option("debug")->isPresent()) {
            sw << "Debugging is enabled\n";
        }
    });

    cli::Command folderDeleteCommand{"delete", "Delete a folder in the workspace"};

    cli::Command folderCommand{"folder", "Manager folders in the plywood workspace"};
    folderCommand.add(std::move(folderListCommand));
    folderCommand.add(std::move(folderDeleteCommand));

    cli::CommandLine cl{"plytool", "Manage plywood workspace"};
    cl.add(cli::Option{"debug", "Enable debugging."}.shortName('d'));
    cl.add(cli::Option("log-filename", "Specify a file to send the log output to.").shortName('l'));
    cl.add(std::move(folderCommand));

    auto context = cl.parse(argc, argv);
    // context.printUsage(&sw);
    context.run(&sw);

    return 0;
#endif // 0

#if 0
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

    // FIXME: Only call getBuildFolders when really needed
    env.buildFolders = build::BuildFolder::getList();
    s32 defaultIndex = find(env.buildFolders.view(), [&](const build::BuildFolder* bf) {
        return bf->buildFolderName == workspace.currentBuildFolder;
    });
    if (defaultIndex >= 0) {
        env.currentBuildFolder = env.buildFolders[defaultIndex];
    }

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
