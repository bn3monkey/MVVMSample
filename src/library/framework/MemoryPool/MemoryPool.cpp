#include "MemoryPool.hpp"

using namespace Bn3Monkey;
Bn3MemoryBlockPools<BLOCK_SIZE_POOL_LENGTH> Bn3MemoryPool::_impl{};