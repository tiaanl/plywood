/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-cli/Context.h>

namespace ply {
namespace cli {

namespace {

template <typename Iterable, typename Functor>
u32 maxWidthOf(const Iterable& iterable, Functor functor) {
    u32 maxWidth = 0;
    for (auto& it : iterable) {
        maxWidth = max(functor(it), maxWidth);
    }
    return maxWidth;
}

} // namespace

int Context::run(StringWriter* sw) {
    PLY_ASSERT(m_lastCommand && m_rootCommand);

    if (!m_errors.isEmpty() || !m_lastCommand) {
        printUsage(sw);
        return 1;
    }

    return m_lastCommand->run(this);
}

void Context::printUsage(StringWriter* sw) const {
    PLY_ASSERT(m_lastCommand && m_rootCommand);

    // Errors

    if (m_errors.numItems() > 0) {
        for (auto& error : m_errors) {
            *sw << error << '\n';
        }
        *sw << '\n';
    }

    // Header with tool name and description

    *sw << m_rootCommand->m_name;
    if (!m_rootCommand->m_description.isEmpty()) {
        *sw << " - " << m_rootCommand->m_description;
    }
    *sw << '\n';

    // Usage line

    *sw << "Usage: " << m_rootCommand->m_name;
    if (!m_optionsByName.isEmpty()) {
        *sw << " [options]";
    }

    for (auto& command : m_passedCommands) {
        *sw << ' ' << command->name();
    }

    auto& children = m_lastCommand->m_subCommands;
    if (!children.isEmpty()) {
        *sw << " <command>";
    }

    for (auto& arg : m_argumentsByIndex) {
        *sw << " <" << arg->m_definition->name() << '>';
    }

    *sw << "\n";

    // Available commands

    if (!children.isEmpty()) {
        *sw << "\nAvailable commands:\n";

        auto maxWidth =
            maxWidthOf(children, [](Command* command) { return command->name().numBytes; });
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

    if (m_argumentsByIndex.numItems() > 0) {
        *sw << "\nArguments:\n";

        auto maxWidth = maxWidthOf(
            m_argumentsByIndex, [](auto& value) { return value->m_definition->name().numBytes; });
        maxWidth += 2;

        for (auto& arg : m_argumentsByIndex) {
            *sw << "  " << arg->m_definition->name();
            for (u32 i = 0; i < maxWidth - arg->m_definition->name().numBytes; ++i) {
                *sw << ' ';
            }
            if (!arg->m_definition->description().isEmpty()) {
                *sw << arg->m_definition->description();
            }
            *sw << '\n';
        }
    }

    // Available options

    struct OptionLine {
        String option;
        StringView description;
    };
    Array<OptionLine> optionLines;
    u32 maxLength = 0;

    if (!m_optionsByName.isEmpty()) {
        *sw << "\nAvailable options:\n";

        for (auto& option : m_optionsByName) {
            StringWriter line;
            line << "  --" << option->m_definition->name();
            if (!option->m_definition->shortName().isEmpty()) {
                line << ", -" << option->m_definition->shortName();
            }
            String l = line.moveToString();
            maxLength = max(maxLength, l.numBytes);
            optionLines.append(std::move(l), option->m_definition->description());
        }

        maxLength += 2;

        for (auto& optionLine : optionLines) {
            *sw << optionLine.option;
            for (u32 i = 0; i < maxLength - optionLine.option.numBytes; ++i) {
                *sw << ' ';
            }
            *sw << optionLine.description << '\n';
        }
    }
}

OptionValue* Context::option(StringView name) const {
    auto cursor = m_optionsByName.find(name);
    if (!cursor.wasFound()) {
        return nullptr;
    }

    return cursor->get();
}

ArgumentValue* Context::argument(StringView name) const {
    auto cursor = m_argumentsByName.find(name);
    if (!cursor.wasFound()) {
        return nullptr;
    }
    return cursor->get();
}

// static
Context Context::build(const Command& rootCommand, ArrayView<const StringView> args) {
    Context result{rootCommand};

    Array<StringView> passedOptions;
    Array<StringView> passedArguments;

    bool foundLastCommand = true;
    for (auto& arg : args.subView(1)) {
        if (arg[0] == '-') {
            passedOptions.append(arg);
        } else {
            if (foundLastCommand) {
                auto command = result.m_lastCommand->findCommand(arg);
                if (command) {
                    result.m_passedCommands.append(command);
                    result.mergeCommand(*command);
                } else {
                    passedArguments.append(arg);
                    foundLastCommand = false;
                }
            } else {
                passedArguments.append(arg);
            }
        }
    }

    // TODO: Handle the case where there are arguments passed that did not resolve to commands.

    result.parseOptions(passedOptions.view());
    result.parseArguments(passedArguments.view());

    // Assume we consumed every single argument we have in the context.
    if (passedArguments.numItems() > result.m_argumentsByIndex.numItems()) {
        auto leftOver = passedArguments.subView(result.m_argumentsByIndex.numItems());
        passedArguments = leftOver;
    }

    return result;
}

Context::Context(const Command& rootCommand) : m_rootCommand{&rootCommand}, m_lastCommand{&rootCommand} {
    mergeCommand(rootCommand);
}

void Context::mergeCommand(const Command& command) {
    m_lastCommand = &command;

    for (auto& option : command.m_options) {
        auto optionValue = Owned<OptionValue>::create(&option);

        auto nameCursor = m_optionsByName.insertOrFind(option.name());
        PLY_ASSERT(!nameCursor.wasFound());
        *nameCursor = std::move(optionValue);

        auto shortNameCursor = m_optionsByShortName.insertOrFind(option.shortName());
        // We allow short names not existing, because short names are optional.
        *shortNameCursor = nameCursor->borrow();
    }

    for (auto& arg : command.m_arguments) {
        auto value = Owned<ArgumentValue>::create(&arg);

        auto nameCursor = m_argumentsByName.insertOrFind(arg.name());
        PLY_ASSERT(!nameCursor.wasFound());
        *nameCursor = std::move(value);

        m_argumentsByIndex.append(nameCursor->borrow());
    }
}

void Context::parseOptions(ArrayView<StringView> options) {
    for (auto& option : options) {
        bool isShortName = option[1] != '-';
        StringView namePart{option[1] == '-' ? option.subStr(2) : option.subStr(1)};
        StringView valuePart;
        auto equalsPos = namePart.findByte('=');
        if (equalsPos >= 1) {
            valuePart = namePart.subStr(equalsPos + 1u);
            namePart = namePart.subStr(0, equalsPos);
        }

        OptionValue* optionValue = nullptr;
        if (isShortName) {
            auto cursor = m_optionsByShortName.find(namePart);
            if (cursor.wasFound()) {
                optionValue = cursor->get();
            }
        } else {
            auto cursor = m_optionsByName.find(namePart);
            if (cursor.wasFound()) {
                optionValue = cursor->get();
            }
        }

        if (!optionValue) {
            m_errors.append(String::format("Invalid option found: {}", option));
            continue;
        }

        optionValue->m_value = valuePart;
        optionValue->m_present = true;
    }
}

void Context::parseArguments(ArrayView<StringView> arguments) {
    for (u32 argIndex = 0; argIndex < min(arguments.numItems, m_argumentsByIndex.numItems());
         ++argIndex) {
        auto& arg = m_argumentsByIndex[argIndex];
        arg->m_value = arguments[argIndex];
    }
}

} // namespace cli
} // namespace ply