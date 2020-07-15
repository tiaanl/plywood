/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <Core.h>
#include <WorkspaceSettings.h>
#include <ply-cli/CommandLine.h>

namespace ply {

namespace build {
struct BuildFolder;
} // namespace build

struct CommandLine {
    Array<StringView> args;
    u32 index = 1;
    Array<StringView> skippedOpts;
    bool finalized = false;

    CommandLine(int argc, char* argv[])
        : args{ArrayView<const char*>{(const char**) argv, (u32) argc}} {
    }
    ~CommandLine() {
        PLY_ASSERT(this->finalized);
    }

    StringView readToken();
    StringView checkForSkippedOpt(const LambdaView<bool(StringView)>& matcher);
    PLY_INLINE bool checkForSkippedOpt(StringView argToMatch) {
        return !this->checkForSkippedOpt([&](StringView arg) { return arg == argToMatch; })
                    .isEmpty();
    }
    void finalize();
};

struct PlyToolCommandEnv {
    CommandLine* cl = nullptr;
    cli::Context* context = nullptr;
    WorkspaceSettings* workspace = nullptr;
    Array<Owned<build::BuildFolder>> buildFolders;
    build::BuildFolder* currentBuildFolder = nullptr;
};

bool prefixMatch(StringView input, StringView cmd, u32 minUnits = 2);
void fatalError(StringView msg);
void ensureTerminated(CommandLine* cl);

struct CommandDescription {
    StringView name;
    StringView description;
};

using CommandList = ArrayView<const CommandDescription>;

void printUsage(StringWriter* sw, CommandList commands);
void printUsage(StringWriter* sw, StringView command, CommandList commands = {});

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
