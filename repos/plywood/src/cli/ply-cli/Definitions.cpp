/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-cli/Definitions.h>

namespace ply {
namespace cli {

void Command::handler(Handler handler) {
    m_handler = std::move(handler);
}

Option& Command::option(StringView name, StringView description) {
    return m_options.append(name, description);
}

void Command::option(Option option) {
    m_options.append(std::move(option));
}

Argument& Command::argument(StringView name, StringView description) {
    return m_arguments.append(name, description);
}

void Command::argument(Argument argument) {
    m_arguments.append(std::move(argument));
}

Command& Command::subCommand(StringView name, StringView description,
                             Functor<void(Command&)> init) {
    auto& command = subCommandInternal({name, description});

    if (init.isValid()) {
        init.call(command);
    }

    return command;
}

void Command::subCommand(Command command) {
    subCommandInternal(std::move(command));
}

Borrowed<Command> Command::findCommand(StringView name) const {
    auto cursor = m_subCommands.find(name);
    if (cursor.wasFound()) {
        return cursor->borrow();
    }

    return {};
}

s32 Command::run(Context* context) const {
    if (m_handler.isValid()) {
        return m_handler.call(context);
    }

    return 1;
}

Command& Command::subCommandInternal(Command&& command) {
    auto cursor = m_subCommands.insertOrFind(command.name());
    PLY_ASSERT(!cursor.wasFound());
    // FIXME: This is a very cumbersome interface.  I don't want to write `new` here.
    *cursor = new Command(std::forward<Command&&>(command));
    return *cursor->get();
}

} // namespace cli
} // namespace ply
