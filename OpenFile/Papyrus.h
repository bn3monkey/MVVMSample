#if !defined(PAPYRUS)
#define PAPYRUS

#include <functional>

class Sans;
namespace TT
{
	using SANSCallback = std::function<void(Sans)>;
}

void registerCallback(TT::SANSCallback callback);
void unregisterCallback();

#endif
