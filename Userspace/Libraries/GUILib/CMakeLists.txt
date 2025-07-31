set(SOURCES
        Context.cpp
        Image.cpp
        Stack.cpp
        View.cpp
        Window.cpp
)

add_library(GUILib STATIC ${SOURCES})

target_include_directories(GUILib PRIVATE ${CMAKE_SOURCE_DIR}/Kernel ${CMAKE_SOURCE_DIR}/Userspace)

target_link_libraries(GUILib PRIVATE SerialisationLib GraphicsLib)
target_link_options(GUILib PRIVATE
    -static
)
