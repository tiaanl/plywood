/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-cli/Definitions.h>

namespace ply {
namespace cli {

Command& Command::handler(Handler handler) {
    m_handler = std::move(handler);
    return *this;
}

Command& Command::add(Option option) {
    m_options.append(std::move(option));
    return *this;
}

Command& Command::add(Command command) {
    auto cursor = m_subCommands.insertOrFind(command.name());
    PLY_ASSERT(!cursor.wasFound());
    *cursor = Owned<Command>::create(std::move(command));
    return *this;
}

Command* Command::findCommand(StringView name) const {
    auto cursor = m_subCommands.find(name);
    if (cursor.wasFound()) {
        return cursor->get();
    }

    return nullptr;
}

} // namespace cli
} // namespace ply