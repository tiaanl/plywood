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

namespace ply {
namespace cli {

class DetailsBase {
public:
    DetailsBase() = default;

    DetailsBase(StringView name, StringView description)
        : m_name{name}, m_description{description} {
    }

    StringView name() const {
        return m_name;
    }

    DetailsBase& name(StringView name) {
        m_name = name;
        return *this;
    }

    StringView description() const {
        return m_description;
    }

    DetailsBase& description(StringView description) {
        m_description = description;
        return *this;
    }

protected:
    String m_name;
    String m_description;
};

class Flag : public DetailsBase {
public:
    Flag() = default;

    Flag(StringView name, StringView description) : DetailsBase{name, description} {
    }

    StringView shortName() const {
        return m_shortName;
    }

    Flag shortName(StringView shortName) {
        m_shortName = shortName;
        return *this;
    }

    StringView defaultValue() const {
        return m_defaultValue;
    }

    Flag defaultValue(StringView defaultValue) {
        m_defaultValue = defaultValue;
        return *this;
    }

private:
    String m_shortName;
    String m_defaultValue;
};

class Argument : public DetailsBase {
public:
    Argument() = default;

    Argument(StringView name, StringView description) : DetailsBase{name, description} {
    }
};

class Command : public DetailsBase {
public:
    Command() = default;

    Command(StringView name, StringView description) : DetailsBase{name, description} {
    }

    Command* findCommand(StringView name) {
        auto cursor = m_commands.find(name);
        if (cursor.wasFound()) {
            return &*cursor;
        }

        return nullptr;
    }

    Command& addCommand(StringView name, StringView description) {
        auto& command = addOption(m_commands, name, description);
        command.m_parent = this;
        return command;
    }

    Flag& addFlag(StringView name, StringView description) {
        return addOption(m_flags, name, description);
    }

    Argument& addArgument(StringView name, StringView description) {
        return addOption(m_arguments, name, description);
    }

private:
    friend class Context;

    template <typename T>
    struct OptionsTraits {
        using Key = StringView;
        using Item = T;

        static Key comparand(const Item& item) {
            return item.name();
        }
    };

    template <typename T>
    T& addOption(HashMap<OptionsTraits<T>>& map, StringView name, StringView description) {
        auto cursor = map.insertOrFind(name);
        PLY_ASSERT(!cursor.wasFound()); // Duplicate entry.
        cursor->name(name);
        cursor->description(description);
        return *cursor;
    }

    Command* m_parent = nullptr;
    HashMap<OptionsTraits<Command>> m_commands;
    HashMap<OptionsTraits<Flag>> m_flags;
    HashMap<OptionsTraits<Argument>> m_arguments;
};

class Context {
public:
    static Context build(ArrayView<StringView> arguments, Command* command) {
        Context result;

        result.merge(command);

        for (u32 i = 1; i < arguments.numItems; ++i) {
            if (arguments[i][0] == '-') {
                continue;
            }

            StdOut::createStringWriter() << "Checking argument: " << arguments[i] << '\n';

            auto c = result.m_leafCommand->findCommand(arguments[i]);
            if (c) {
                result.merge(c);
            } else {
                // TODO: Handle command not found!
                break;
            }
        }

        return result;
    }

    Context() = default;

    Command* command() const {
        return m_leafCommand;
    }

    void printUsage(StringWriter* sw) {
        Command* rootCommand = m_leafCommand;
        while (rootCommand->m_parent) {
            rootCommand = rootCommand->m_parent;
        }
        if (!rootCommand || !m_leafCommand) {
            // TODO: This should really be an assert, but static analysis complains about null
            // dereferencing.
            return;
        }

        *sw << rootCommand->m_name << '\n';
        if (!rootCommand->m_description.isEmpty()) {
            *sw << rootCommand->m_description << '\n';
        }

        *sw << "\nUsage: " << rootCommand->m_name;
        if (!m_flagsByName.isEmpty()) {
            *sw << " <flags>";
        }

        Array<StringView> commandList;
        for (Command* current = m_leafCommand; current->m_parent; current = current->m_parent) {
            commandList.append(current->name());
        }
        std::reverse(std::begin(commandList), std::end(commandList));
        for (auto& command : commandList) {
            *sw << ' ' << command;
        }

        auto& children = m_leafCommand->m_commands;

        if (!children.isEmpty()) {
            *sw << " <command>";
        }
        *sw << "\n";

        if (!children.isEmpty()) {
            *sw << "\nAvailable commands:\n";

            u32 maxWidth = 0;
            for (auto& child : children) {
                maxWidth = max(child.m_name.numBytes, maxWidth);
            }
            maxWidth += 2;

            for (auto& child : children) {
                *sw << "  " << child.m_name;
                for (u32 i = 0; i < maxWidth - child.m_name.numBytes; ++i) {
                    *sw << ' ';
                }
                if (!child.m_description.isEmpty()) {
                    *sw << child.m_description;
                }
                *sw << '\n';
            }
        }

        Array<Tuple<String, StringView>> flagLines;
        u32 maxLength = 0;

        if (!m_flagsByName.isEmpty()) {
            *sw << "\nAvailable flags:\n";

            for (auto& flag : m_flagsByName) {
                StringWriter line;
                line << "  --" << flag->name();
                if (!flag->shortName().isEmpty()) {
                    line << ", -" << flag->shortName();
                }
                String l = line.moveToString();
                maxLength = max(maxLength, l.numBytes);
                flagLines.append(std::move(l), flag->description());
            }

            maxLength += 2;

            for (auto& flagLine : flagLines) {
                *sw << flagLine.first;
                for (u32 i = 0; i < maxLength - flagLine.first.numBytes; ++i) {
                    *sw << ' ';
                }
                *sw << flagLine.second << '\n';
            }
        }
    }

private:
    struct FlagsByNameTraits {
        using Key = StringView;
        using Item = Flag*;

        static Key comparand(const Item& item) {
            return item->name();
        }
    };

    struct FlagsByShortNameTraits {
        using Key = StringView;
        using Item = Flag*;

        static Key comparand(const Item& item) {
            return item->shortName();
        }
    };

    void merge(Command* command) {
        m_leafCommand = command;

        for (auto& flag : command->m_flags) {
            auto cursor = m_flagsByName.insertOrFind(flag.name());
            PLY_ASSERT(!cursor.wasFound());
            *cursor = &flag;

            if (!flag.shortName().isEmpty()) {
                auto shortCursor = m_flagsByShortName.insertOrFind(flag.shortName());
                PLY_ASSERT(!shortCursor.wasFound());
                *shortCursor = &flag;
            }
        }
    }

    Command* m_leafCommand;
    HashMap<FlagsByNameTraits> m_flagsByName;
    HashMap<FlagsByShortNameTraits> m_flagsByShortName;
};

class CL : public Command {
public:
    CL(StringView name, StringView description) : Command{name, description} {
    }

    Context parse(int argc, char* argv[]) {
        Array<StringView> args = ArrayView<const char*>{(const char**) argv, (u32) argc};
        return parse(args.view());
    }

private:
    Context parse(ArrayView<StringView> args) {
        auto context = Context::build(args, this);

        return context;
    }
};

} // namespace cli
} // namespace ply

int main(int argc, char* argv[]) {
    using namespace ply;
    using namespace ply::cli;

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

#if 0
    auto cl = CL{"plytool", "Manage plywood workspace"};
    cl.addFlag("debug", "Enable debugging");
    auto& folderCommand = cl.addCommand("folder", "Manager folders in the plywood workspace");
    auto& folderListCommand = folderCommand.addCommand("list", "List folders in the workspace");
    folderListCommand.addFlag("verbose", "Enable verbosity").defaultValue("1");
    folderCommand.addCommand("delete", "Delete a folder in the workspace");

    auto sw = StdOut::createStringWriter();

    auto context = cl.parse(argc, argv);
    context.printUsage(&sw);

    sw << "Found command: " << context.command()->name() << '\n';

    return 0;
#endif  // 0

#if 1
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
