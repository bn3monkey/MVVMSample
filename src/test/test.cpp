#include "framework/MemoryPool/test.hpp"
#include "framework/StaticVector/test.hpp"
#include "framework/StaticString/test.hpp"
#include "framework/ScopedTaskRunner/test.hpp"
#include "framework/AsyncProperty/test.hpp"

int main()
{
    testStaticString(false);
    testStaticVector(false);
    testMemoryPool(false);
    testScopedTaskRunner(false);
    testAsyncProperty(true);
    
    return 0;
}
