/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-cli/CommandLine.h>

namespace ply {
namespace cli {

Context CommandLine::parse(int argc, char* argv[]) {
    Array<StringView> args{ArrayView<const char*>{(const char**) argv, (u32) argc}};
    return parse(args.view());
}

Context CommandLine::parse(ArrayView<const StringView> args) {
    return Context::build(*this, args);
}

} // namespace cli
} // namespace ply
