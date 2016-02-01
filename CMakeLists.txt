cmake_minimum_required(VERSION 3.2)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(autograph CXX)

############## Hack no1 ##############
if(UNIX AND "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -lc++abi")
        #include_directories("/usr/include/c++/v1/")
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# TODO if boost is not found, download it and compile it as an external project
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost COMPONENTS date_time chrono wave thread filesystem system REQUIRED)
link_directories(${Boost_LIBRARY_DIR})

add_subdirectory(ext/)

file(GLOB SHADERPP_SOURCES src/shaderpp/*.cpp)
file(GLOB SHADERPP_HEADERS src/shaderpp/*.hpp)
source_group("Source files\\shaderpp" FILES ${SHADERPP_SOURCES})
source_group("Header files\\shaderpp" FILES ${SHADERPP_HEADERS})

file(GLOB AG_SOURCES_CORE src/autograph/*.cpp)
file(GLOB AG_SOURCES_OPENGL src/autograph/backend/opengl/*.cpp)
source_group("Source files\\Core" FILES ${AG_SOURCES_CORE})
source_group("Source files\\OpenGL" FILES  ${AG_SOURCES_OPENGL})

file(GLOB AG_HEADERS_CORE src/autograph/*.hpp)
file(GLOB AG_HEADERS_OPENGL src/autograph/backend/opengl/*.hpp)
source_group("Header files\\Core" FILES ${AG_HEADERS_CORE})
source_group("Header files\\OpenGL" FILES ${AG_HEADERS_OPENGL})

add_library(autograph STATIC ${AG_SOURCES_CORE} ${AG_SOURCES_OPENGL})

include_directories(src)
include_directories(ext/filesystem)
include_directories(ext/glm)
include_directories(ext/GSL/include)

#set(AG_INCLUDE_DIR src/)
#set(FILESYSTEM_INCLUDE_DIR ext/filesystem)
#set(GLM_INCLUDE_DIR ext/glm)
#set(GSL_INCLUDE_DIR ext/GSL/include)

add_library(shaderpp STATIC ${SHADERPP_SOURCES})
target_link_libraries(shaderpp ${Boost_LIBRARIES})
target_include_directories(shaderpp PUBLIC ext/GSL/include ${Boost_INCLUDE_DIR})

target_link_libraries(autograph shaderpp cppformat glloadgen glfw ${Boost_LIBRARIES} ${GLFW_LIBRARIES})
target_include_directories(autograph PUBLIC src ext/glm ext/GSL/include)

############## Extras ##############
function(target_link_autograph_extra)
    #cmake_parse_arguments(AG_SAMPLE "" "TARGET;DIRECTORY" "REQUIRES" ${ARGN} )
    #file(GLOB SAMPLE_SOURCES ${AG_SAMPLE_DIRECTORY}/*.cpp)
    #add_executable(${AG_SAMPLE_TARGET} ${AG_SAMPLE_COMMON_SOURCES} ${SAMPLE_SOURCES})
    #target_link_libraries(${AG_SAMPLE_TARGET} autograph ${AG_SAMPLE_REQUIRES})
    #target_include_directories(${AG_SAMPLE_TARGET} PRIVATE ${AG_INCLUDE_DIR})
endfunction()

function(add_extra)
	cmake_parse_arguments(AG_EXTRA "" "TARGET" "REQUIRES" ${ARGN} )
	set(EXTRA_DIR src/extra/${AG_EXTRA_TARGET})
	file(GLOB EXTRA_SOURCES ${EXTRA_DIR}/*.cpp ${EXTRA_DIR}/*.c)
	file(GLOB EXTRA_HEADERS ${EXTRA_DIR}/*.hpp)
	if (EXTRA_SOURCES)
		add_library(${AG_EXTRA_TARGET} STATIC ${EXTRA_SOURCES})
	else() 
		add_library(${AG_EXTRA_TARGET} INTERFACE)
	endif()
	target_include_directories(${AG_EXTRA_TARGET} INTERFACE src/extra/)
	target_link_libraries(${AG_EXTRA_TARGET} ${AG_EXTRA_REQUIRES})
endfunction()

add_extra(TARGET image_io REQUIRES autograph stb)

add_subdirectory(examples)

