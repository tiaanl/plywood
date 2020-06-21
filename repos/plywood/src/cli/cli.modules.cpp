#include <ply-build-repo/Module.h>

// [ply module="cli"]
void module_ply_cli(ModuleArgs* args) {
    args->addSourceFiles("ply-cli");
    args->addIncludeDir(Visibility::Public, ".");
    args->addTarget(Visibility::Public, "runtime");
}
