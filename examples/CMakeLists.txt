#file(GLOB AG_SAMPLE_COMMON_SOURCES common/sample.cpp)

function(autograph_add_sample)
    cmake_parse_arguments(AG_SAMPLE "" "TARGET;DIRECTORY" "REQUIRES" ${ARGN} )
    file(GLOB SAMPLE_SOURCES ${AG_SAMPLE_DIRECTORY}/*.cpp)
    add_executable(${AG_SAMPLE_TARGET} ${AG_SAMPLE_COMMON_SOURCES} ${SAMPLE_SOURCES})
    target_link_libraries(${AG_SAMPLE_TARGET} autograph ${AG_SAMPLE_REQUIRES})
    target_include_directories(${AG_SAMPLE_TARGET} PRIVATE ${AG_INCLUDE_DIR})
endfunction()

autograph_add_sample(TARGET simple DIRECTORY simple REQUIRES assimp docopt_s)
