#ifndef XPOSLIB_EVENTLISTENER_H
#define XPOSLIB_EVENTLISTENER_H

#include "API/Event.h"

namespace xpOS::OSLib
{

class EventListener
{
public:
    EventListener();
    EventListener(const EventListener&) = delete;
    void add(uint64_t pd, xpOS::API::EventTypeMask mask);
    void remove(uint64_t pd, xpOS::API::EventTypeMask mask);
    int listen(xpOS::API::Event* resultOut, int maxEvents);
    
private:
    uint64_t m_pd;
};

}

#endif