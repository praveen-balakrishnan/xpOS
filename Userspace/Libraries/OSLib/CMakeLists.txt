set(SOURCES
        EventListener.cpp
        Pipe.cpp
        Socket.cpp
)

add_library(OSLib STATIC ${SOURCES})

target_include_directories(OSLib PRIVATE ${CMAKE_SOURCE_DIR}/Kernel ${CMAKE_SOURCE_DIR}/Userspace)
