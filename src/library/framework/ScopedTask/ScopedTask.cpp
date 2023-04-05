#include "ScopedTask.hpp"

using namespace Bn3Monkey;

std::shared_ptr<ScopedTaskRunnerImpl> runner;
std::shared_ptr<ScopedTaskScopeImplPool> scope_pool;

#ifdef __BN3MONKEY_MEMORY_POOL__
#define MAKE_SHARED(TYPE, TAG, ...) Bn3Monkey::makeSharedFromMemoryPool<TYPE>(TAG, __VA_ARGS__)
#else
#define MAKE_SHARED(TYPE, TAG, ...) std::shared_ptr<TYPE>(new TYPE(__VA_ARGS__))
#endif



ScopedTaskScopeImpl& ScopedTaskScope::getScope(const Bn3Tag& scope_name)
{
    assert(scope_pool != nullptr);

    auto& ret = scope_pool->getScope(scope_name, 
        // onStart
        runner->onStart()
    );
    return ret;
}
ScopedTaskScope::ScopedTaskScope(const Bn3Tag& scope_name) : _impl(getScope(scope_name))
{
}

bool ScopedTaskRunner::initialize()
{
    scope_pool = MAKE_SHARED(ScopedTaskScopeImplPool, Bn3Tag("global_scope_pools"));
    runner = MAKE_SHARED(ScopedTaskRunnerImpl, Bn3Tag("global_task_runner"));

    return runner->initialize( scope_pool->onClear());
}
void ScopedTaskRunner::release()
{
    runner->release();
    
    scope_pool.reset();
    runner.reset();
}
