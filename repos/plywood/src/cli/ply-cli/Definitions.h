/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/container/Array.h>
#include <ply-runtime/string/String.h>
#include <ply-runtime/container/HashMap.h>
#include <ply-runtime/container/Functor.h>
#include <ply-runtime/container/Owned.h>

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

class Argument : public BaseDefinition {
public:
    Argument(StringView name, StringView description) : BaseDefinition{name, description} {
    }

    StringView defaultValue() const {
        return m_defaultValue;
    }

    Argument& defaultValue(StringView defaultValue) {
        m_defaultValue = defaultValue;
        return *this;
    }

private:
    String m_defaultValue;
};

class Command : public BaseDefinition {
public:
    using Handler = Functor<s32(Context*)>;

    // This constructor is only public, because HashMap needs to default construct objects on
    // insert.
    Command() : BaseDefinition{"", ""} {
    }

    Command(StringView name, StringView description) : BaseDefinition{name, description} {
    }

    /*!
    Set the handler for this command.
    */
    void handler(Handler handle);

    /*!
    \beginGroup
    Add an option to this command.
    */
    Option& option(StringView name, StringView description);
    void option(Option option);

    /*!
    \beginGroup
    Add an argument to this command.
    */
    Argument& argument(StringView name, StringView description);
    void argument(Argument argument);

    /*!
    \beginGroup
    Add a sub command to this command.
    */
    Command& subCommand(StringView name, StringView description, Functor<void(Command&)> init);
    void subCommand(Command command);

    /*!
    Returns a sub command with the specified name, otherwise nullptr.
    */
    Command* findCommand(StringView name) const;

    /*!
    Run the command's handler if it is valid, otherwise return 1.
    */
    s32 run(Context* context);

private:
    friend class Context;

    struct SubCommandsTraits {
        using Key = StringView;
        using Item = Owned<Command>;

        static Key comparand(const Item& item) {
            return item->name();
        }
    };

    Command& subCommandInternal(Command&& command);

    Handler m_handler;
    Array<Option> m_options;
    Array<Argument> m_arguments;
    HashMap<SubCommandsTraits> m_subCommands;
};

} // namespace cli
} // namespace ply
