/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/string/StringView.h>
#include <ply-runtime/string/String.h>
#include <ply-runtime/container/HashMap.h>
#include <ply-runtime/io/text/StringWriter.h>

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

class CommandLine : public Command {
public:
    CommandLine(StringView name, StringView description) : Command{name, description} {
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
