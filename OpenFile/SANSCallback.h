#ifndef SANSCALLBACK
#define SANSCALLBACK

#include "SANS.h"
#include <functional>

namespace TT
{
	using SANSCallback = std::function<void(Sans)>;
}

#endif