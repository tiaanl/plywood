#include <ply-build-repo/Module.h>

// [ply module="cli"]
void module_ply_cli(ModuleArgs* args) {
    args->addSourceFiles("ply-cli");
    args->addIncludeDir(Visibility::Public, ".");
    args->addTarget(Visibility::Public, "runtime");
}

// [ply extern="catch2" provider="header"]
ExternResult extern_catch2_header(ExternCommand cmd, ExternProviderArgs* args) {
    // Toolchain filters
    if (args->toolchain->get("targetPlatform")->text() != "windows") {
        return {ExternResult::UnsupportedToolchain, "Target platform must be 'windows'"};
    }
    if (findItem(ArrayView<const StringView>{"x86", "x64"}, args->toolchain->get("arch")->text()) <
        0) {
        return {ExternResult::UnsupportedToolchain, "Target arch must be 'x86' or 'x64'"};
    }
    if (args->providerArgs) {
        return {ExternResult::BadArgs, ""};
    }

    // https://github.com/catchorg/Catch2/releases/download/v2.12.2/catch.hpp

    StringView version = "2.12.2";

    // Handle Command
    Tuple<ExternResult, ExternFolder*> er =
        args->findExistingExternFolder(args->toolchain->get("arch")->text());
    if (cmd == ExternCommand::Status) {
        return er.first;
    } else if (cmd == ExternCommand::Install) {
        if (er.first.code != ExternResult::SupportedButNotInstalled) {
            return er.first;
        }
        ExternFolder* externFolder = args->createExternFolder(args->toolchain->get("arch")->text());
        String archivePath = NativePath::join(externFolder->path, "catch.hpp");
        String url = String::format(
            "https://github.com/catchorg/Catch2/releases/download/v{}/catch.hpp", version);
        if (!downloadFile(archivePath, url)) {
            return {ExternResult::InstallFailed, String::format("Error downloading '{}'", url)};
        }
        externFolder->success = true;
        externFolder->save();
        return {ExternResult::Installed, ""};
    } else if (cmd == ExternCommand::Instantiate) {
        if (er.first.code != ExternResult::Installed) {
            return er.first;
        }
        String installFolder = er.second->path;
        args->dep->includeDirs.append(installFolder);
        return {ExternResult::Instantiated, ""};
    }
    PLY_ASSERT(0);
    return {ExternResult::Unknown, ""};
}

// [ply module="cli-tests"]
void module_ply_cli_tests(ModuleArgs* args) {
    args->buildTarget->targetType = BuildTargetType::EXE;
    args->addSourceFiles("ply-cli-tests");
    args->addTarget(Visibility::Public, "cli");
    args->addExtern(Visibility::Public, "catch2");
}
