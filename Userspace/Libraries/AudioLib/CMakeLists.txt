set(SOURCES
        WAV/WAV.cpp
)

add_library(AudioLib STATIC ${SOURCES})

target_include_directories(AudioLib PRIVATE ${CMAKE_SOURCE_DIR}/Kernel ${CMAKE_SOURCE_DIR}/Userspace)
