#include "ScopedTask.hpp"

using namespace Bn3Monkey;

ScopedTaskRunnerImpl runner;
ScopedTaskScopeImplPool scope_pool;

ScopedTaskScopeImpl& ScopedTaskScope::getScope(const char* scope_name)
{
    auto* ret = scope_pool.getScope(scope_name, 
        // onStart
        [&](ScopedTaskScopeImpl& self){
            return runner.start(self);
        }, 
        // onStop
        [&](ScopedTaskScopeImpl& self){
            return runner.stop(self);
        });
    return *ret;
}
ScopedTaskScope::ScopedTaskScope(const char* scope_name) : _impl(getScope(scope_name))
{
}

bool ScopedTaskRunner::initialize()
{
    return runner.initialize([&]() {
        scope_pool.clearScope();
        });
}
void ScopedTaskRunner::release()
{
    runner.release();
}
