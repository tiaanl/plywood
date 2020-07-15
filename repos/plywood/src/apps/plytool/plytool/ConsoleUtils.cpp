/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <Core.h>

namespace ply {

void fatalError(StringView msg) {
    StringWriter stdErr = StdErr::createStringWriter();
    stdErr << "error: " << msg;
    if (!msg.endsWith("\n")) {
        stdErr << '\n';
    }
    stdErr.flushMem();
    exit(1);
}

} // namespace ply
