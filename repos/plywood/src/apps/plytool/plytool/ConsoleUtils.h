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

class Command {
public:
    Command(StringView name, StringView description = {}, ArrayView<const Command> children = {})
        : m_name{name}, m_description{description}, m_children{children} {
    }

    const String& name() const {
        return m_name;
    }

    void addCommand(Command&& command) {
        m_children.append(command);
    }

    friend static bool operator==(const Command& command, const StringView& other) {
        return command.m_name == other;
    }

protected:
    friend class CommandLine2;

    String m_name;
    String m_description;
    Array<Command> m_children;
};

class CommandLine2 : public Command {
public:
    struct Context {
        Array<StringView> commands;
        Command* command;
    };

    CommandLine2(StringView executable, StringView description = {})
        : Command{executable, description} {
    }

    Context parse(int argc, char** argv) {
        // We skip the first argument, because it is the name of the executable.
        Array<StringView> arguments{ArrayView<const char*>{(const char**) argv, (u32) argc}};
        auto context = buildContext(arguments.subView(1), this);

        PLY_ASSERT(context.command);

        // If we did not find a leaf command, then we print usage.
        if (context.command->m_children.numItems() > 0) {
            auto sw = StdOut::createStringWriter();
            printUsage(&sw, context);
        }

        return context;
    }

    void printUsage(StringWriter* sw, const Context& context) {
        *sw << m_name << '\n';
        if (!m_description.isEmpty()) {
            *sw << m_description << '\n';
        }

        auto& children = context.command->m_children;

        *sw << "\nUsage:";
        for (auto& command : context.commands) {
            *sw << ' ' << command;
        }
        if (!children.isEmpty()) {
            *sw << " <command>";
        }
        *sw << "\n";

        if (!children.isEmpty()) {
            *sw << "\nAvailable commands:\n\n";

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
    }

private:
    void mergeIntoContext(Context* context, Command* command) {
        context->commands.append(command->m_name);
        context->command = command;

        StdOut::createStringWriter() << "merging command: " << command->m_name << '\n';
    }

    Context buildContext(ArrayView<StringView> arguments, Command* command) {
        auto errors = StdErr::createStringWriter();

        Context result;
        mergeIntoContext(&result, command);

        for (u32 i = 0; i < arguments.numItems; ++i) {
            errors << "Checking argument: " << arguments[i] << '\n';
            errors.flush();

            if (arguments[i][0] == '-') {
                continue;
            }

            auto pos = findItem(result.command->m_children.view(), arguments[i]);
            if (pos >= 0) {
                mergeIntoContext(&result, &result.command->m_children[pos]);
            } else {
                errors << "Command not found (" << arguments[i] << ")\n";
                break;
            }
        }

        return result;
    }
};

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
