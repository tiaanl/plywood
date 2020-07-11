/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <web-common/Server.h>
#include <ply-web-serve-docs/DocServer.h>
#include <WebServer/Config.h>
#include <web-common/FetchFromFileSystem.h>
#include <web-common/SourceCode.h>
#include <web-common/Echo.h>
#include <ply-cli/CommandLine.h>

using namespace ply;
using namespace web;

struct AllParams {
    DocServer docs;
    FetchFromFileSystem fileSys;
    SourceCode sourceCode;
};

void myRequestHandler(AllParams* params, StringView requestPath, ResponseIface* responseIface) {
    if (requestPath.startsWith("/static/")) {
        FetchFromFileSystem::serve(&params->fileSys, requestPath, responseIface);
    } else if (requestPath.startsWith("/file/")) {
        SourceCode::serve(&params->sourceCode, requestPath.subStr(6), responseIface);
    } else if (requestPath.rtrim([](char c) { return c == '/'; }) == "/echo") {
        echo_serve(nullptr, requestPath, responseIface);
    } else if (requestPath.startsWith("/docs/")) {
        params->docs.serve(requestPath.subStr(6), responseIface);
    } else if (requestPath == "/") {
        params->docs.serve("", responseIface);
    } else {
        responseIface->respondGeneric(ResponseCode::NotFound);
    }
}

void writeMsgAndExit(StringView msg) {
    StringWriter stdErr = StdErr::createStringWriter();
    stdErr << "Error: " << msg;
    if (!msg.endsWith("\n")) {
        stdErr << '\n';
    }
    stdErr.flushMem();
    exit(1);
}

struct CommandLine {
    Array<StringView> args;
    u32 index = 1;

    CommandLine(int argc, char* argv[])
        : args{ArrayView<const char*>({(const char**) argv, (u32) argc})} {
    }

    StringView readToken() {
        if (this->index >= this->args.numItems())
            return {};
        return args[index++];
    }
};

int webServerMain(cli::Context* context) {
    Socket::initialize(IPAddress::V6);

    StringView dataRoot = context->argument("data-root")->value();
    u16 port = context->option("port")->to<u16>();

#if PLY_TARGET_POSIX
    if (port == 0) {
        if (const char* portCStr = getenv("PORT")) {
            u16 p = StringView{portCStr}.to<u16>();
            if (p == 0) {
                writeMsgAndExit(String::format(
                    "Invalid port number {} found in environment variable PORT", portCStr));
            }
        }
    }
    if (port == 0) {
        if (const char* dataRootCStr = getenv("WEBSERVER_DOC_DIR")) {
            dataRoot = dataRootCStr;
        }
    }
#endif
    StdOut::createStringWriter().format("Serving from {} on port {}\n", dataRoot, port);
    AllParams allParams;
    allParams.fileSys.rootDir = dataRoot;
    allParams.docs.init(dataRoot);
    allParams.sourceCode.rootDir = NativePath::normalize(PLY_WORKSPACE_FOLDER);
    if (!runServer(port, {&allParams, myRequestHandler})) {
        return 1;
    }
    Socket::shutdown();
    return 0;
}

int main(int argc, char* argv[]) {
    auto cl = cli::CommandLine{"WebServer", "Serve the plywood documentation."};
    cl.add(cli::Option{"port", "The port used for listening to incoming connections"}
               .shortName("p")
               .defaultValue(String::from(WEBSERVER_DEFAULT_PORT)));
    cl.add(cli::Argument{"data-root", "The root directory to serve files from"}.defaultValue(
        WEBSERVER_DEFAULT_DOC_DIR));
    cl.handler(webServerMain);

    auto sw = StdOut::createStringWriter();
    return cl.parse(argc, argv).run(&sw);
}
