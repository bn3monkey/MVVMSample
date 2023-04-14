#include "ScopedTask.hpp"

using namespace Bn3Monkey;

std::shared_ptr<ScopedTaskScopeImplPool> scope_pool;
std::shared_ptr<ScopedTaskLooperScheduler> looper_scheduler;

#ifdef __BN3MONKEY_MEMORY_POOL__
#define MAKE_SHARED(TYPE, TAG, ...) Bn3Monkey::makeSharedFromMemoryPool<TYPE>(TAG, __VA_ARGS__)
#else
#define MAKE_SHARED(TYPE, TAG, ...) std::shared_ptr<TYPE>(new TYPE(__VA_ARGS__))
#endif



ScopedTaskScopeImpl& ScopedTaskScope::getScope(const Bn3Tag& scope_name)
{
    assert(scope_pool != nullptr);

    auto& ret = scope_pool->getScope(scope_name);
    return ret;
}
ScopedTaskScope::ScopedTaskScope(const Bn3Tag& scope_name) : _impl(getScope(scope_name))
{
}

ScopedTaskLooperImpl& ScopedTaskLooper::getLooper(const Bn3Tag& looper_name)
{
    assert(looper_scheduler != nullptr);

    auto& ret = looper_scheduler->getLooper(looper_name);
    return ret;
}
ScopedTaskLooper::ScopedTaskLooper(const Bn3Tag& looper_name) : _impl(getLooper(looper_name))
{

}

bool ScopedTaskRunner::initialize()
{
    scope_pool = MAKE_SHARED(ScopedTaskScopeImplPool, Bn3Tag("global_scope_pools"));
    if (!scope_pool)
        return false;

    looper_scheduler = MAKE_SHARED(ScopedTaskLooperScheduler, Bn3Tag("global_looper_scheduler"));
    if (!looper_scheduler)
        return false;

    looper_scheduler->start();
    return true;
}
void ScopedTaskRunner::release()
{

    looper_scheduler->stop();
    looper_scheduler.reset();

    scope_pool->release();
    scope_pool.reset();    
}
