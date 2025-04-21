set(SOURCES
        DeserialisedData.cpp
        SerialisedData.cpp
)

add_library(SerialisationLib STATIC ${SOURCES})

target_include_directories(SerialisationLib PRIVATE ${CMAKE_SOURCE_DIR}/Kernel ${CMAKE_SOURCE_DIR}/Userspace)
