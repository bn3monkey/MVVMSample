#include "framework/MemoryPool/test.hpp"
#include "framework/ScopedTaskRunner/test.hpp"


int main()
{
    testMemoryPool(true);
    testScopedTaskRunner(true);
    
    return 0;
}
