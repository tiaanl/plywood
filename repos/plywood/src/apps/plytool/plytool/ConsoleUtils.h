/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <Core.h>
#include <WorkspaceSettings.h>
#include <ply-runtime/algorithm/Find.h>

namespace ply {

namespace build {
struct BuildFolder;
} // namespace build

struct CommandLine {
    Array<StringView> args;
    u32 index = 1;
    Array<StringView> skippedOpts;
    bool finalized = false;

    CommandLine(int argc, char* argv[])
        : args{ArrayView<const char*>{(const char**) argv, (u32) argc}} {
    }
    ~CommandLine() {
        PLY_ASSERT(this->finalized);
    }

    StringView readToken();
    StringView checkForSkippedOpt(const LambdaView<bool(StringView)>& matcher);
    PLY_INLINE bool checkForSkippedOpt(StringView argToMatch) {
        return !this->checkForSkippedOpt([&](StringView arg) { return arg == argToMatch; })
                    .isEmpty();
    }
    void finalize();
};

namespace cli {

class Flag {
public:
    Flag(StringView name, StringView description) : m_name{name}, m_description{description} {
    }

    void defaultValue(StringView defaultValue) {
        m_defaultValue = defaultValue;
    }

    void shortName(StringView shortName) {
        m_shortName = shortName;
    }

private:
    friend struct Context;
    friend class CommandLine2;

    String m_name;
    String m_description;

    String m_defaultValue;
    String m_shortName;
};

class CommandBase {
public:
    void flag(StringView name, StringView description, Functor<void(Flag*)> init = {}) {
        Flag& result = m_flags.append(name, description);
        if (init.isValid()) {
            init.call(&result);
        }
    }

    void flag(StringView name, Functor<void(Flag*)> init = {}) {
        flag(name, {}, std::move(init));
    }

protected:
    friend struct Context;

    CommandBase(StringView name, StringView description, Array<Owned<CommandBase>> children = {})
        : m_name{name}, m_description{description}, m_children{std::move(children)} {
    }

    friend static bool operator==(const Owned<CommandBase>& command, const StringView& other) {
        return command->m_name == other;
    }

    friend class CommandLine2;

    String m_name;
    String m_description;
    Array<Owned<CommandBase>> m_children;
    Array<Flag> m_flags;
};

class Command : public CommandBase {
public:
    Command(StringView name, StringView description) : CommandBase{name, description, {}} {
    }
};

class CommandGroup : public CommandBase {
public:
    CommandGroup(StringView name, StringView description,
                 const ArrayView<CommandBase>& commands = {})
        : CommandBase{name, description} {
        m_children.reserve(commands.numItems);
        for (auto& command : commands) {
            auto e = Owned<CommandBase>::create(command);
        }
    }

    void command(StringView name, StringView description, Functor<void(Command*)> init = {}) {
        auto result = Owned<Command>::create(name, description);
        if (init.isValid()) {
            init.call(result.get());
        }
        m_children.append(std::move(result));
    }

    void command(StringView name, Functor<void(Command*)> init = {}) {
        command(name, {}, std::move(init));
    }

    void group(StringView name, StringView description, Functor<void(CommandGroup*)> init = {}) {
        auto result = Owned<CommandGroup>::create(name, description);
        if (init.isValid()) {
            init.call(result.get());
        }
        m_children.append(std::move(result));
    }

    void group(StringView name, Functor<void(CommandGroup*)> init = {}) {
        group(name, {}, std::move(init));
    }
};

struct Context {
    struct FlagsTraitsBase {
        using Key = StringView;
        using Item = Flag*;
    };

    struct FlagsTraits : public FlagsTraitsBase {
        static Key comparand(const Item& item) {
            return item->m_name;
        }
    };

    struct ShortFlagsTraits : public FlagsTraitsBase {
        static Key comparand(const Item& item) {
            return item->m_shortName;
        }
    };

    Array<Borrowed<CommandBase>> commands;
    Borrowed<CommandBase> lastCommand;

    HashMap<FlagsTraits> flags;
    HashMap<ShortFlagsTraits> shortFlags;

    bool isRunnable() const {
        return lastCommand->m_children.isEmpty();
    }

    void run() {
        StdOut::createStringWriter() << "Running: " << lastCommand->m_name;
        // TODO: Runnit
    }

    void printUsage(StringWriter* sw) {
        auto rootCommand = commands[0];
        PLY_ASSERT(rootCommand && lastCommand);

        *sw << rootCommand->m_name << '\n';
        if (!rootCommand->m_description.isEmpty()) {
            *sw << rootCommand->m_description << '\n';
        }

        auto& children = lastCommand->m_children;

        *sw << "\nUsage: " << commands[0]->m_name;
        if (!flags.isEmpty()) {
            *sw << " <flags>";
        }

        for (auto& command : commands.subView(1)) {
            *sw << ' ' << command->m_name;
        }

        if (!children.isEmpty()) {
            *sw << " <command>";
        }
        *sw << "\n";

        if (!children.isEmpty()) {
            *sw << "\nAvailable commands:\n";

            u32 maxWidth = 0;
            for (auto& child : children) {
                maxWidth = max(child->m_name.numBytes, maxWidth);
            }
            maxWidth += 2;

            for (auto& child : children) {
                *sw << "  " << child->m_name;
                for (u32 i = 0; i < maxWidth - child->m_name.numBytes; ++i) {
                    *sw << ' ';
                }
                if (!child->m_description.isEmpty()) {
                    *sw << child->m_description;
                }
                *sw << '\n';
            }
        }

        Array<Tuple<String, StringView>> flagLines;
        u32 maxLength = 0;

        if (!flags.isEmpty()) {
            *sw << "\nAvailable flags:\n";

            for (auto& flag : flags) {
                StringWriter line;
                line << "  --" << flag->m_name;
                if (!flag->m_shortName.isEmpty()) {
                    line << ", -" << flag->m_shortName;
                }
                String l = line.moveToString();
                maxLength = max(maxLength, l.numBytes);
                flagLines.append(std::move(l), flag->m_description);
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
};

class CommandLine2 : public CommandGroup {
public:
    CommandLine2(StringView executable, StringView description = {})
        : CommandGroup{executable, description} {
    }

    Context parse(int argc, char** argv) {
        // We skip the first argument, because it is the name of the executable.
        Array<StringView> arguments{ArrayView<const char*>{(const char**) argv, (u32) argc}};
        auto context = buildContext(arguments.subView(1), this);

        PLY_ASSERT(context.lastCommand);

        return context;
    }

private:
    void mergeIntoContext(Context* context, CommandBase* command) {
        context->commands.append(command);
        context->lastCommand = command;

        for (auto& flag : command->m_flags) {
            auto cursor = context->flags.insertOrFind(flag.m_name);
            PLY_ASSERT(!cursor.wasFound());
            *cursor = &flag;

            if (!flag.m_shortName.isEmpty()) {
                auto shortCursor = context->shortFlags.insertOrFind(flag.m_shortName);
                PLY_ASSERT(!shortCursor.wasFound());
                *shortCursor = &flag;
            }
        }
    }

    Context buildContext(ArrayView<StringView> arguments, CommandBase* command) {
        Context result;
        mergeIntoContext(&result, command);

        for (u32 i = 0; i < arguments.numItems; ++i) {
            if (arguments[i][0] == '-') {
                continue;
            }

            auto pos = findItem(result.lastCommand->m_children.view(), arguments[i]);
            if (pos >= 0) {
                mergeIntoContext(&result, result.lastCommand->m_children[pos].borrow());
            } else {
                // TODO: Handle command not found!
                break;
            }
        }

        return result;
    }
};

inline CommandLine2 commandLine(StringView name, StringView description,
                                Functor<void(CommandLine2*)> init) {
    auto result = CommandLine2(name, description);
    init.call(&result);
    return result;
}

} // namespace cli

struct PlyToolCommandEnv {
    CommandLine* cl = nullptr;
    WorkspaceSettings* workspace = nullptr;
    Array<Owned<build::BuildFolder>> buildFolders;
    build::BuildFolder* currentBuildFolder = nullptr;
};

bool prefixMatch(StringView input, StringView cmd, u32 minUnits = 2);
void fatalError(StringView msg);
void ensureTerminated(CommandLine* cl);

struct CommandDescription {
    StringView name;
    StringView description;
};

using CommandList = ArrayView<const CommandDescription>;

void printUsage(StringWriter* sw, CommandList commands);
void printUsage(StringWriter* sw, StringView command, CommandList commands = {});

} // namespace ply
