set(SOURCES
        Graphics.cpp
        Image.cpp
        Text.cpp
)

add_library(GraphicsLib STATIC ${SOURCES})

target_include_directories(GraphicsLib PRIVATE ${CMAKE_SOURCE_DIR}/Kernel ${CMAKE_SOURCE_DIR}/Userspace)
