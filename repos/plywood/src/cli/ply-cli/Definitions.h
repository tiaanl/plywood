#pragma once
#include <ply-runtime/container/Array.h>
#include <ply-runtime/string/String.h>
#include <ply-runtime/container/HashMap.h>

namespace ply {
namespace cli {

class BaseDefinition {
public:
    BaseDefinition(StringView name, StringView description)
        : m_name{name}, m_description{description} {
    }

    const String& name() const {
        return m_name;
    }

    const String& description() const {
        return m_description;
    }

protected:
    String m_name;
    String m_description;
};

template <typename T>
class DefinitionWithShortName : public BaseDefinition {
public:
    DefinitionWithShortName(StringView name, StringView description)
        : BaseDefinition{name, description} {
    }

    const String& shortName() const {
        return m_shortName;
    }

    T& setShortName(StringView shortName) {
        m_shortName = shortName;
        return static_cast<T&>(*this);
    }

protected:
    String m_shortName;
};

class FlagDefinition : public DefinitionWithShortName<FlagDefinition> {
public:
    FlagDefinition(StringView name, StringView description)
        : DefinitionWithShortName{name, description} {
    }
};

class OptionDefinition : public DefinitionWithShortName<FlagDefinition> {
public:
    OptionDefinition(StringView name, StringView description)
        : DefinitionWithShortName{name, description} {
    }
};

class ArgumentDefinition : public BaseDefinition {
public:
    ArgumentDefinition(StringView name, StringView description)
        : BaseDefinition{name, description}, m_required{false} {
    }

    bool isRequired() const {
        return m_required;
    }

    ArgumentDefinition& setRequired(bool required) {
        m_required = required;
        return *this;
    }

private:
    bool m_required;
};

class CommandDefinition : public BaseDefinition {
public:
    // This constructor is only public, because HashMap needs to default construct objects on
    // insert.
    CommandDefinition() : BaseDefinition{"", ""} {
    }

    CommandDefinition(StringView name, StringView description) : BaseDefinition{name, description} {
    }

    FlagDefinition& addFlag(StringView name, StringView description);
    OptionDefinition& addOption(StringView name, StringView description);
    ArgumentDefinition& addArgument(StringView name, StringView description);
    CommandDefinition& addSubCommand(StringView name, StringView description);
    CommandDefinition* findCommand(StringView name) const;

private:
    friend class Context;

    struct SubCommandsTraits {
        using Key = StringView;
        using Item = CommandDefinition;

        static Key comparand(const Item& item) {
            return item.name();
        }
    };

    Array<FlagDefinition> m_flags;
    Array<OptionDefinition> m_options;
    Array<ArgumentDefinition> m_arguments;
    HashMap<SubCommandsTraits> m_subCommands;
};

} // namespace cli
} // namespace ply
