#include "doomkeys.h"
#include "doomgeneric.h"
#include "API/Event.h"
#include "API/Syscall.h"
#include "API/SharedMemory.h"
#include "API/HID.h"
#include "Libraries/IPCLib/Connection.h"
#include "Libraries/OSLib/Socket.h"
#include "Libraries/OSLib/Pipe.h"
#include "System/WindowServer/ClientEndpoint.h"
#include "System/WindowServer/ServerEndpoint.h"
#include <pthread.h>
#include <string>

using namespace xpOS;
using namespace xpOS::System::WindowServer;

static uint64_t t_since_start = 0;
static xpOS::IPC::Connection<ClientEndpoint>* conn = nullptr;
static uint32_t* framebuffer = nullptr;

static constexpr int KEYQUEUE_SIZE = 16;
static unsigned short s_KeyQueue[KEYQUEUE_SIZE];
static unsigned int s_KeyQueueWriteIndex = 0;
static unsigned int s_KeyQueueReadIndex = 0;

static unsigned char convertToDoomKey(unsigned char key)
{
    using namespace xpOS::API::HID;
    switch (key) {
    case ControlKeys::ESCAPE:
        key = KEY_ESCAPE;
        break;
    case ControlKeys::RETURN:
        key = KEY_ENTER;
        break;
    case ControlKeys::LARROW:
        key = KEY_LEFTARROW;
        break;
    case ControlKeys::RARROW:
        key = KEY_RIGHTARROW;
        break;
    case ControlKeys::UARROW:
        key = KEY_UPARROW;
        break;
    case ControlKeys::DARROW:
        key = KEY_DOWNARROW;
        break;
    case ControlKeys::CTRL:
        key = KEY_FIRE;
        break;
    case ' ':
        key = KEY_USE;
    case ControlKeys::SHIFT:
        key = KEY_RSHIFT;
        break;
    default:
        key = std::tolower(key);
        break;
    }
    
    return key;
}

static void addKeyToQueue(int pressed, unsigned char keyCode)
{
	unsigned char key = convertToDoomKey(keyCode);

	unsigned short keyData = (pressed << 8) | key;

	s_KeyQueue[s_KeyQueueWriteIndex] = keyData;
	s_KeyQueueWriteIndex++;
	s_KeyQueueWriteIndex %= KEYQUEUE_SIZE;
}

struct ClientHandler
{
    static void handle_ipc_message(
        KeyEvent keyEvent,
        xpOS::IPC::Connection<ClientEndpoint>& conn
    ) 
    {
        addKeyToQueue(!keyEvent.keyUp, keyEvent.key);
    }
};

void* receive_events(void*) {
    conn->handle_messages_indefinitely<ClientHandler>();
}

extern "C" void DG_Init()
{
    int serverPd = OSLib::popen("socket::local", 0, static_cast<API::EventType>(OSLib::LocalSocket::Flags::NON_BLOCKING));
    while (!OSLib::LocalSocket::connect(serverPd, "xp::WindowServer"));
    conn = new IPC::Connection<ClientEndpoint>(serverPd);

    CreateWindow cw = {
        .width = DOOMGENERIC_RESX,
        .height = DOOMGENERIC_RESY
    };

    auto callback = conn->send_and_wait_for_reply<CreateWindow, CreateWindowCallback>(cw);
    xpOS::API::SharedMemory::LinkRequest req = {
        .str = callback.sharedMemoryId.c_str(),
        .mappedTo = nullptr,
        .length = DOOMGENERIC_RESX * DOOMGENERIC_RESY * 4
    };

    int sharedMemPd = xpOS::OSLib::popen("shared_mem", &req);
    framebuffer = reinterpret_cast<uint32_t*>(req.mappedTo);
    t_since_start = xpOS::API::Syscalls::syscall(SYSCALL_BOOTTICKS_MS);

    pthread_t thr;
    pthread_create(&thr, NULL, receive_events, nullptr);
}

extern "C" void DG_DrawFrame()
{
    memcpy(framebuffer, DG_ScreenBuffer, DOOMGENERIC_RESX * DOOMGENERIC_RESY * 4);
    for (int i = 0; i < DOOMGENERIC_RESX * DOOMGENERIC_RESY; i++)
        framebuffer[i] |= 0xFF000000;

    FlushWindow flush;

    if (conn)
        conn->send_message(flush);
}

extern "C"  void DG_SetWindowTitle(const char * title) {}

extern "C" void DG_SleepMs(uint32_t ms)
{
    xpOS::API::Syscalls::syscall(SYSCALL_SLEEP_FOR, ms);
}

extern "C" uint32_t DG_GetTicksMs()
{
    return xpOS::API::Syscalls::syscall(SYSCALL_BOOTTICKS_MS) - t_since_start;
}

extern "C" int DG_GetKey(int* pressed, unsigned char* doomKey)
{
    if (s_KeyQueueReadIndex == s_KeyQueueWriteIndex)
	{
		//key queue is empty

		return 0;
	}
	else
	{
		unsigned short keyData = s_KeyQueue[s_KeyQueueReadIndex];
		s_KeyQueueReadIndex++;
		s_KeyQueueReadIndex %= KEYQUEUE_SIZE;

		*pressed = keyData >> 8;
		*doomKey = keyData & 0xFF;

		return 1;
	}
}

extern "C" int main()
{
    doomgeneric_Create(0, 0);
    for (int i = 0; ;i++)
    {
        doomgeneric_Tick();
    }
    while(1);
}