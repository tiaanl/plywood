/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <Core.h>
#include <ConsoleUtils.h>
#include <ply-build-folder/BuildFolder.h>
#include <ply-build-repo/Repo.h>
#include <ply-build-repo/RepoRegistry.h>
#include <ply-runtime/algorithm/Sort.h>
#include <ply-build-repo/BuildInstantiatorDLLs.h>
#include <ply-cli/CommandLine.h>

namespace ply {

s32 module_listHandler(PlyToolCommandEnv*) {
    using namespace build;

    PLY_SET_IN_SCOPE(RepoRegistry::instance_, RepoRegistry::create());

    StringWriter sw = StdOut::createStringWriter();
    Array<const Repo*> repos;
    for (const Repo* repo : RepoRegistry::get()->repos) {
        repos.append(repo);
    }
    sort(repos.view(), [](const Repo* a, const Repo* b) { //
        return a->repoName < b->repoName;
    });
    for (const Repo* repo : repos) {
        sw.format("Modules in repo '{}':\n", repo->repoName);
        Array<TargetInstantiator*> targetInsts;
        for (TargetInstantiator* targetInst : repo->targetInstantiators) {
            targetInsts.append(targetInst);
        }
        sort(targetInsts.view(), [](const TargetInstantiator* a, const TargetInstantiator* b) {
            return a->name < b->name;
        });
        for (TargetInstantiator* targetInst : targetInsts) {
            sw.format("    {}\n", targetInst->name);
        }
        sw << "\n";
    }

    return 0;
}

s32 module_updateHandler(PlyToolCommandEnv*) {
    using namespace build;

    buildInstantiatorDLLs(true);

    return 0;
}

void buildCommand_module(cli::Command* root, PlyToolCommandEnv* env) {
    cli::Command listCmd{"list", "List all the modules available in the workspace."};
    listCmd.handler(wrapHandler(env, module_listHandler));

    cli::Command updateCmd{"update", "Update the module registry for the workspace."};
    updateCmd.handler(wrapHandler(env, module_updateHandler));

    cli::Command cmd{"module", "Manage modules in the workspace."};
    cmd.add(std::move(listCmd));
    cmd.add(std::move(updateCmd));

    root->add(std::move(cmd));
}

} // namespace ply
