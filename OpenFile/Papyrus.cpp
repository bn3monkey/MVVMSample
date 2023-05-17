#include "Papyrus.h"

TT::SANSCallback _callback;

void registerCallback(TT::SANSCallback callback)
{
	_callback = callback;
}

void unregisterCallback()
{
	_callback = nullptr;
}
