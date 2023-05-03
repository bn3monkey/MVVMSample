#ifndef __BN3MONKEY_ASYNC_PROPERTY_NODE__
#define __BN3MONKEY_ASYNC_PROPERTY_NODE__

namespace Bn3Monkey
{
	class AsyncPropertyNode
	{

	};

	struct AsyncPropertyExtendedNode
	{
		AsyncPropertyNode* node{ nullptr };
		size_t size{0};
	};
}

#endif