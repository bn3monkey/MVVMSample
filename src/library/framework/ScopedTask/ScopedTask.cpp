#include "ScopedTask.hpp"

using namespace Bn3Monkey;

ScopedTaskRunnerImpl* runner{ nullptr };
ScopedTaskScopeImplPool* scope_pool{nullptr};

#ifdef __BN3MONKEY_MEMORY_POOL__
#define ALLOC(TYPE, TAG) Bn3Monkey::Bn3MemoryPool::construct<TYPE>(TAG)
#define DEALLOC(PTR) Bn3Monkey::Bn3MemoryPool::destroy(PTR)
#else
#define ALLOC(TYPE, TAG) new TYPE()
#define DEALLOC(PTR) delete PTR
#endif




ScopedTaskScopeImpl& ScopedTaskScope::getScope(const char* scope_name)
{
    assert(scope_pool != nullptr);

    auto* ret = scope_pool->getScope(scope_name, 
        // onStart
        runner->onStart()
    );
    return *ret;
}
ScopedTaskScope::ScopedTaskScope(const char* scope_name) : _impl(getScope(scope_name))
{
}

bool ScopedTaskRunner::initialize()
{
    if (scope_pool == nullptr)
        scope_pool = ALLOC(ScopedTaskScopeImplPool, Bn3Tag("TaskScopePool"));
    if (runner == nullptr)
        runner = ALLOC(ScopedTaskRunnerImpl, Bn3Tag("TaskRunner"));

    return runner->initialize(scope_pool->onTimeout(), scope_pool->onClear());
}
void ScopedTaskRunner::release()
{
    runner->release();
    if (scope_pool != nullptr)
    {
        DEALLOC(scope_pool);
        scope_pool = nullptr;
    }
    if (runner != nullptr)
    {
        DEALLOC(runner);
        runner = nullptr;
    }
}
