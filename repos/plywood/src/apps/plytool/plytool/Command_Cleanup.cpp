/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <Core.h>
#include <ConsoleUtils.h>
#include <ply-cpp/Preprocessor.h>
#include <ply-cpp/ErrorFormatting.h>
#include <ply-cli/CommandLine.h>

namespace ply {

void checkFileHeader(StringView srcPath, StringView desiredHeader) {
    String src = FileSystem::native()->loadTextAutodetect(srcPath).first;
    if (FileSystem::native()->lastResult() != FSResult::OK)
        return;

    using namespace cpp;
    PPVisitedFiles visitedFiles;
    Preprocessor pp;
    pp.visitedFiles = &visitedFiles;

    // FIXME: The following code is duplicated in several places.
    // Factor it out into a convenience function.
    u32 sourceFileIdx = visitedFiles.sourceFiles.numItems();
    PPVisitedFiles::SourceFile& srcFile = visitedFiles.sourceFiles.append();
    srcFile.absPath = srcPath;
    srcFile.contents = std::move(src);
    srcFile.fileLocMap = FileLocationMap::fromView(srcFile.contents);

    u32 includeChainIdx = visitedFiles.includeChains.numItems();
    PPVisitedFiles::IncludeChain& includeChain = visitedFiles.includeChains.append();
    includeChain.isMacroExpansion = 0;
    includeChain.fileOrExpIdx = sourceFileIdx;

    Preprocessor::StackItem& item = pp.stack.append();
    item.includeChainIdx = includeChainIdx;
    item.strViewReader = StringViewReader{srcFile.contents};
    pp.linearLocAtEndOfStackTop = srcFile.contents.numBytes;

    PPVisitedFiles::LocationMapTraits::Item locMapItem;
    locMapItem.linearLoc = 0;
    locMapItem.includeChainIdx = includeChainIdx;
    locMapItem.offset = 0;
    visitedFiles.locationMap.insert(std::move(locMapItem));

    bool anyError = false;
    pp.errorHandler = [&](Owned<BaseError>&&) { anyError = true; };
    Token tok;
    for (;;) {
        tok = readToken(&pp);
        if (tok.type != Token::LineComment && tok.type != Token::CStyleComment)
            break;
    }

    ExpandedFileLocation efl = expandFileLocation(&visitedFiles, tok.linearLoc);
    PLY_ASSERT(efl.srcFile == &srcFile);
    StringView existingFileHeader = StringView::fromRange(
        srcFile.contents.bytes, tok.identifier.bytes - efl.fileLoc.numBytesIntoLine);

    // Trim header at first blank line
    {
        StringViewReader svr{existingFileHeader};
        while (StringView line = svr.readView<fmt::Line>()) {
            if (line.rtrim(isWhite).isEmpty()) {
                existingFileHeader = StringView::fromRange(existingFileHeader.bytes, line.bytes);
                break;
            }
        }
    }

    StringView restOfFile =
        StringView::fromRange(existingFileHeader.end(), srcFile.contents.view().end());
    String withHeader = desiredHeader + restOfFile;
    FileSystem::native()->makeDirsAndSaveTextIfDifferent(srcPath, withHeader,
                                                         TextFormat::platformPreference());
}

void cleanupRepo(StringView repoPath, StringView desiredHeader, StringView clangFormatPath = {}) {
    for (WalkTriple& triple : FileSystem::native()->walk(repoPath)) {
        for (const WalkTriple::FileInfo& file : triple.files) {
            if (file.name.endsWith(".cpp") || file.name.endsWith(".h")) {
                // Run clang-format
                if (clangFormatPath) {
                    Owned<Subprocess> sub =
                        Subprocess::exec(clangFormatPath, {"-i", file.name}, triple.dirPath,
                                         Subprocess::Output::inherit());
                    if (sub) {
                        sub->join();
                    }
                }

                // Check file header
                checkFileHeader(NativePath::join(triple.dirPath, file.name), desiredHeader);
            }
        }
    }
}

s32 cleanup_handler(PlyToolCommandEnv*) {
    StringView desiredHeader = R"(/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
)";
    cleanupRepo(NativePath::join(PLY_WORKSPACE_FOLDER, "repos/plywood/src"), desiredHeader);

    return 0;
}

void buildCommand_cleanup(cli::Command* root, PlyToolCommandEnv* env) {
    root->subCommand("cleanup", "Clean up repository",
                     [env](auto& c) { c.handler(wrapHandler(env, cleanup_handler)); });
}

} // namespace ply
