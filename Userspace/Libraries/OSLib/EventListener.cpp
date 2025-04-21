#include "API/Event.h"
#include "Libraries/OSLib/Pipe.h"
#include "Libraries/OSLib/EventListener.h"
#include "API/Syscall.h"

namespace xpOS::OSLib
{
    EventListener::EventListener()
    {
        m_pd = xpOS::OSLib::popen("elistener");
    }

    void EventListener::add(uint64_t targetPd, xpOS::API::EventTypeMask mask)
    {
        xpOS::API::Syscalls::syscall(SYSCALL_ELISTENER_ADD, m_pd, targetPd, mask);
    }
    
    void EventListener::remove(uint64_t targetPd, xpOS::API::EventTypeMask mask)
    {
        xpOS::API::Syscalls::syscall(SYSCALL_ELISTENER_REMOVE, m_pd, targetPd, mask);
    }

    int EventListener::listen(xpOS::API::Event* resultOut, int maxEvents)
    {
        return xpOS::OSLib::pread(m_pd, resultOut, maxEvents * sizeof(xpOS::API::Event)) / sizeof(xpOS::API::Event);
    }
}