#pragma once
#include <ply-cli/Definitions.h>

namespace ply {
namespace cli {

class Context {
public:
    bool foundLeafCommand() const {
        return m_lastCommand->m_subCommands.isEmpty();
    }
private:
    friend class CommandLine;

    struct Flag {
        FlagDefinition* definition;
        bool isPresent;
    };

    struct FlagsByNameTraits {
        using Key = StringView;
        using Item = Flag;

        static Key comparand(const Item& item) {
            return item.definition->name();
        }
    };

    struct FlagsByShortNameTraits {
        using Key = StringView;
        using Item = Flag;

        static Key comparand(const Item& item) {
            return item.definition->shortName();
        }
    };

    struct Option {
        OptionDefinition* definition;
        String value;
    };

    struct OptionsByNameTraits {
        using Key = StringView;
        using Item = Option;

        static Key comparand(const Item& item) {
            return item.definition->name();
        }
    };

    struct OptionsByShortNameTraits {
        using Key = StringView;
        using Item = Option;

        static Key comparand(const Item& item) {
            return item.definition->shortName();
        }
    };

    struct Argument {
        ArgumentDefinition* definition;
        String value;
    };

    struct ArgumentsTraits {
        using Key = StringView;
        using Item = Argument;

        static Key comparand(const Item& item) {
            return item.definition->name();
        }
    };

    static Context build(CommandDefinition* rootCommand, ArrayView<const StringView> args) {
        Context result{rootCommand};

        for (u32 i = 1; i < args.numItems; ++i) {
            if (args[i][0] == '-') {
                continue;
            }

            auto c = result.m_lastCommand->findCommand(args[i]);
            if (c) {
                result.append(c);
            } else {
                // TODO: Handle command not found!
                break;
            }
        }

        parseArgs(args);

        return result;
    }

    explicit Context(CommandDefinition* rootCommand)
        : m_rootCommand{rootCommand}, m_lastCommand{rootCommand} {
        append(rootCommand);
    }

    void append(CommandDefinition* command) {
        m_lastCommand = command;

        for (auto& flag : command->m_flags) {
            {
                auto cursor = m_flagsByName.insertOrFind(flag.name());
                PLY_ASSERT(!cursor.wasFound());
                cursor->definition = &flag;
            }

            {
                auto cursor = m_flagsByShortName.insertOrFind(flag.shortName());
                PLY_ASSERT(!cursor.wasFound());
                cursor->definition = &flag;
            }
        }

        for (auto& option : command->m_options) {
            {
                auto cursor = m_optionsByName.insertOrFind(option.name());
                PLY_ASSERT(!cursor.wasFound());
                cursor->definition = &option;
            }

            {
                auto cursor = m_optionsByShortName.insertOrFind(option.shortName());
                PLY_ASSERT(!cursor.wasFound());
                cursor->definition = &option;
            }
        }

        for (auto& argument : command->m_arguments) {
            auto cursor = m_arguments.insertOrFind(argument.name());
            PLY_ASSERT(!cursor.wasFound());
            cursor->definition = &argument;
        }
    }

    void parseArgs(ArrayView<const StringView> args) {
        for (u32 i = 1; i < args.numItems; ++i) {
            if (args[i].startsWith("--")) {
                auto pos = args[i].findByte('=', 2);
                if (pos >= 0) {
                    // Option
                    auto cursor = m_optionsByName.find(args[i].subStr(2, pos - 2));
                    if (cursor.wasFound()) {
                        cursor->value = args[0].subStr(pos);
                    } else {
                        // TODO: Handle invalid option.
                    }
                } else {
                    // Flag
                    auto cursor = m_flagsByName.find(args[i].subStr(2));
                    if (cursor.wasFound()) {
                        cursor->isPresent = true;
                    } else {
                        // TODO: Handle invalid flag.
                    }
                }
            }
        }
    }

    CommandDefinition* m_rootCommand;
    CommandDefinition* m_lastCommand;

    HashMap<FlagsByNameTraits> m_flagsByName;
    HashMap<FlagsByShortNameTraits> m_flagsByShortName;
    HashMap<OptionsByNameTraits> m_optionsByName;
    HashMap<OptionsByShortNameTraits> m_optionsByShortName;
    HashMap<ArgumentsTraits> m_arguments;
};

class CommandLine : public CommandDefinition {
public:
    CommandLine(StringView name, StringView description) : CommandDefinition{name, description} {
    }

    Context parse(ArrayView<const StringView> args) {
        return Context::build(this, args);
    }
};

} // namespace cli
} // namespace ply
