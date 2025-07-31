set(SOURCES 
        Player.cpp
)

add_executable(MusicPlayer ${SOURCES})

target_link_libraries(MusicPlayer PRIVATE GraphicsLib GUILib OSLib)
target_include_directories(MusicPlayer PRIVATE ${CMAKE_SOURCE_DIR}/Kernel ${CMAKE_SOURCE_DIR}/Userspace)
target_link_options(MusicPlayer PRIVATE
    -static
)
target_compile_options(MusicPlayer PRIVATE -mno-red-zone)

file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/Targets/x86_64/xpinitrd/Applications")
set_target_properties(MusicPlayer PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Targets/x86_64/xpinitrd/Applications")
