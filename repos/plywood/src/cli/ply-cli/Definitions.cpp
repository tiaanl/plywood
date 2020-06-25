#include <ply-cli/Definitions.h>

namespace ply {
namespace cli {

FlagDefinition& CommandDefinition::addFlag(StringView name, StringView description) {
    return m_flags.append(name, description);
}

OptionDefinition& CommandDefinition::addOption(StringView name, StringView description) {
    return m_options.append(name, description);
}

ArgumentDefinition& CommandDefinition::addArgument(StringView name, StringView description) {
    return m_arguments.append(name, description);
}

CommandDefinition& CommandDefinition::addSubCommand(StringView name, StringView description) {
    auto cursor = m_subCommands.insertOrFind(name);
    PLY_ASSERT(!cursor.wasFound());
    *cursor = {name, description};
    return *cursor;
}

CommandDefinition* CommandDefinition::findCommand(StringView name) const {
    auto cursor = m_subCommands.find(name);
    if (cursor.wasFound()) {
        return &*cursor;
    }

    return nullptr;
}

} // namespace cli
} // namespace ply