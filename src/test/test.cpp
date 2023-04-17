#include "framework/MemoryPool/test.hpp"
#include "framework/StaticVector/test.hpp"
#include "framework/ScopedTaskRunner/test.hpp"
#include "framework/AsyncProperty/test.hpp"

int main()
{
    testStaticVector(false);
    testMemoryPool(false);
    testScopedTaskRunner(false);
    testAsyncProperty(true);
    
    return 0;
}
