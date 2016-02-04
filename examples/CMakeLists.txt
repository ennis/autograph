#file(GLOB AG_SAMPLE_COMMON_SOURCES common/sample.cpp)

function(autograph_add_sample)
    cmake_parse_arguments(AG_SAMPLE "" "TARGET" "REQUIRES;SOURCES" ${ARGN} )
    file(GLOB SAMPLE_SOURCES ${AG_SAMPLE_SOURCES})
    add_executable(${AG_SAMPLE_TARGET} ${AG_SAMPLE_COMMON_SOURCES} ${SAMPLE_SOURCES})
    target_link_libraries(${AG_SAMPLE_TARGET} autograph ${AG_SAMPLE_REQUIRES})
    target_include_directories(${AG_SAMPLE_TARGET} PRIVATE ${AG_INCLUDE_DIR})
endfunction()

# The target executable and directory cannot have the same name.
# (otherwise, it will fail at build time with linker complaining that the target file is a directory) 
# Doesn't happen on windows (executables have the .exe suffix)
# Ah, cmake, you never cease to amaze me.
autograph_add_sample(TARGET sample_simple SOURCES simple/*.cpp simple/imgui/*.cpp REQUIRES input image_io assimp rxcpp docopt_s)
autograph_add_sample(TARGET sample_input SOURCES input/*.cpp REQUIRES input image_io rxcpp assimp)
