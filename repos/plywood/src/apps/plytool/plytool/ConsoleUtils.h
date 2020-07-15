/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <Core.h>
#include <WorkspaceSettings.h>

namespace ply {

namespace build {
struct BuildFolder;
} // namespace build

namespace cli {
class Context;
} // namespace cli

struct PlyToolCommandEnv {
    cli::Context* context = nullptr;
    WorkspaceSettings* workspace = nullptr;
    Array<Owned<build::BuildFolder>> buildFolders;
    build::BuildFolder* currentBuildFolder = nullptr;
};

void fatalError(StringView msg);

auto wrapHandler(PlyToolCommandEnv* env, Functor<s32(PlyToolCommandEnv*)> handler) {
    return [env, &handler](cli::Context* context) {
        env->context = context;
        if (handler.isValid()) {
            return handler.call(env);
        }

        return 1;
    };
}

} // namespace ply
