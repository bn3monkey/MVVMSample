#include "framework/MemoryPool/test.hpp"
#include "framework/ScopedTaskRunner/test.hpp"


int main()
{
    testMemoryPool(false);
    testScopedTaskRunner(true);
    
    return 0;
}
