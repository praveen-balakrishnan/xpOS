set(SOURCES 
        HID.cpp
        InfiniteDesktop.cpp
        Graphics.cpp
        Server.cpp
        Screen.cpp
        Window.cpp
)

add_executable(WindowServer ${SOURCES})

target_link_libraries(WindowServer PRIVATE OSLib SerialisationLib)
target_include_directories(WindowServer PRIVATE ${CMAKE_SOURCE_DIR}/Kernel ${CMAKE_SOURCE_DIR}/Userspace)
target_link_options(WindowServer PRIVATE -static)

target_compile_options(WindowServer PRIVATE -mno-red-zone -O2)

file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/Targets/x86_64/xpinitrd/System")
set_target_properties(WindowServer PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Targets/x86_64/xpinitrd/System")
