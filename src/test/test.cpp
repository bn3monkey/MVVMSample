#include "framework/MemoryPool/test.hpp"
#include "framework/ScopedTaskRunner/test.hpp"
#include "framework/AsyncProperty/test.hpp"

int main()
{
    testMemoryPool(false);
    testScopedTaskRunner(false);
    testAsyncProperty(true);
    
    return 0;
}
