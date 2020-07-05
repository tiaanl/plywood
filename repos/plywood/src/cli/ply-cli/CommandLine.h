/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-cli/Definitions.h>
#include <ply-cli/Context.h>

namespace ply {

struct StringWriter;

namespace cli {

class CommandLine : public Command {
public:
    CommandLine(StringView name, StringView description) : Command{name, description} {
    }

    Context parse(int argc, char* argv[]);
    Context parse(ArrayView<const StringView> args);
};

} // namespace cli
} // namespace ply
