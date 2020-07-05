/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-cli/Context.h>

namespace ply {
namespace cli {

void Context::run(StringWriter* sw) {
    PLY_ASSERT(m_lastCommand && m_rootCommand);

    if (!m_errors.isEmpty() || !m_lastCommand || !m_lastCommand->m_handler.isValid()) {
        printUsage(sw);
        return;
    }

    m_lastCommand->m_handler.call(this);
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
    *sw << "\n";

    // Available commands

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

ArgumentValue* Context::argument(u32 index) const {
    PLY_UNUSED(index);
    return nullptr;
}

// static
Context Context::build(Command* rootCommand, ArrayView<const StringView> args) {
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
                    result.append(command);
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

    return result;
}

Context::Context(Command* rootCommand) : m_rootCommand{rootCommand}, m_lastCommand{rootCommand} {
    append(rootCommand);
}

void Context::append(Command* command) {
    m_lastCommand = command;

    for (auto& option : command->m_options) {
        auto optionValue = Owned<OptionValue>::create(&option);

        auto nameCursor = m_optionsByName.insertOrFind(option.name());
        PLY_ASSERT(!nameCursor.wasFound());
        *nameCursor = std::move(optionValue);

        auto shortNameCursor = m_optionsByShortName.insertOrFind(option.shortName());
        PLY_ASSERT(!shortNameCursor.wasFound());
        *shortNameCursor = nameCursor->borrow();
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

} // namespace cli
} // namespace ply
