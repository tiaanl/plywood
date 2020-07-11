/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/io/text/StringWriter.h>
#include <ply-cli/Definitions.h>

namespace ply {
namespace cli {

class CommandLine;
class Context;

class OptionValue {
public:
    OptionValue() : m_definition{nullptr}, m_value{}, m_present{false} {
    }

    explicit OptionValue(Option* definition)
        : m_definition{definition}, m_value{}, m_present{false} {
    }

    StringView value() const {
        return m_value.isEmpty() ? m_definition->defaultValue() : m_value.view();
    }

    template <typename T>
    T to() const {
        return value().to<T>();
    }

    bool isPresent() const {
        return m_present;
    }

private:
    friend class Context;

    Option* m_definition;
    String m_value;
    bool m_present;
};

class ArgumentValue {
public:
    ArgumentValue() : m_definition{nullptr}, m_value{}, m_present{false} {
    }

    explicit ArgumentValue(Argument* definition)
        : m_definition{definition}, m_value{}, m_present{false} {
    }

    StringView value() const {
        return m_value.isEmpty() ? m_definition->defaultValue() : m_value.view();
    }

    template <typename T>
    T to() const {
        return m_value.to<T>();
    }

    bool isPresent() const {
        return m_present;
    }

private:
    friend class Context;

    Argument* m_definition;
    String m_value;
    bool m_present = false;
};

class Context {
public:
    // TODO: Should we pass StringWriter to run?  What is the purpose other than passing it to
    // printUsage if there is no handlers.
    int run(StringWriter* sw);
    void printUsage(StringWriter* sw) const;

    OptionValue* option(StringView name) const;
    ArgumentValue* argument(StringView name) const;

private:
    friend class cli::CommandLine;

    struct OptionsByNameTraits {
        using Key = StringView;
        using Item = Owned<OptionValue>;

        static Key comparand(const Item& item) {
            return item->m_definition->name();
        }
    };

    struct OptionsByShortNameTraits {
        // TODO: This should be `char` but HashMapTraits doesn't like it.
        using Key = StringView;
        using Item = Borrowed<OptionValue>;

        static Key comparand(const Item& item) {
            return item->m_definition->shortName();
        }
    };

    struct ArgumentsByNameTraits {
        using Key = StringView;
        using Item = Owned<ArgumentValue>;

        static Key comparand(const Item& item) {
            return item->m_definition->name();
        }
    };

    static Context build(Command* rootCommand, ArrayView<const StringView> args);

    explicit Context(Command* rootCommand);

    void append(Command* command);
    void parseOptions(ArrayView<StringView> options);
    void parseArguments(ArrayView<StringView> arguments);

    Command* m_rootCommand;
    Command* m_lastCommand;

    Array<Command*> m_passedCommands;

    HashMap<OptionsByNameTraits> m_optionsByName;
    HashMap<OptionsByShortNameTraits> m_optionsByShortName;
    HashMap<ArgumentsByNameTraits> m_argumentsByName;
    Array<Borrowed<ArgumentValue>> m_argumentsByIndex;

    Array<String> m_errors;
};

} // namespace cli
} // namespace ply
