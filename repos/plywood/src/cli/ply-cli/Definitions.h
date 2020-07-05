/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/container/Array.h>
#include <ply-runtime/string/String.h>
#include <ply-runtime/container/HashMap.h>
#include <ply-runtime/container/Functor.h>

namespace ply {
namespace cli {

class Context;

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

class Option : public BaseDefinition {
public:
    Option(StringView name, StringView description) : BaseDefinition{name, description} {
    }

    StringView shortName() const {
        return m_shortName;
    }

    Option& shortName(StringView shortName) {
        m_shortName = shortName;
        return *this;
    }

    StringView defaultValue() const {
        return m_defaultValue;
    }

    Option& defaultValue(StringView defaultValue) {
        m_defaultValue = defaultValue;
        return *this;
    }

protected:
    String m_shortName;
    String m_defaultValue;
};

class Command : public BaseDefinition {
public:
    using Handler = Functor<void(Context*)>;

    // This constructor is only public, because HashMap needs to default construct objects on
    // insert.
    Command() : BaseDefinition{"", ""} {
    }

    Command(StringView name, StringView description) : BaseDefinition{name, description} {
    }

    Command& handler(Handler handle);
    Command& add(Option option);
    Command& add(Command command);

    Command* findCommand(StringView name) const;

private:
    friend class Context;

    struct SubCommandsTraits {
        using Key = StringView;
        using Item = Command;

        static Key comparand(const Item& item) {
            return item.name();
        }
    };

    Handler m_handler;
    Array<Option> m_options;
    HashMap<SubCommandsTraits> m_subCommands;
};

} // namespace cli
} // namespace ply
