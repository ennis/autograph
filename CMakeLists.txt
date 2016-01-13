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
find_package(Boost COMPONENTS wave thread filesystem system REQUIRED)

add_subdirectory(ext/)

file(GLOB SHADERPP_SOURCES src/shaderpp/*.cpp)
file(GLOB SHADERPP_HEADERS src/shaderpp/*.hpp)
source_group("Source files\\shaderpp" FILES ${SHADERPP_SOURCES})
source_group("Header files\\shaderpp" FILES ${SHADERPP_HEADERS})

file(GLOB AG_SOURCES_CORE src/*.cpp)
file(GLOB AG_SOURCES_OPENGL src/backend/opengl/*.cpp)
file(GLOB AG_SOURCES_OPENGL_FX src/backend/opengl/fx/*.cpp)
source_group("Source files\\Core" FILES ${AG_SOURCES_CORE})
source_group("Source files\\OpenGL" FILES  ${AG_SOURCES_OPENGL})
source_group("Source files\\OpenGL\\FX" FILES ${AG_SOURCES_OPENGL_FX})

file(GLOB AG_HEADERS_CORE src/*.hpp)
file(GLOB AG_HEADERS_OPENGL src/backend/opengl/*.hpp)
file(GLOB AG_HEADERS_OPENGL_FX src/backend/opengl/fx/*.hpp)
source_group("Header files\\Core" FILES ${AG_HEADERS_CORE})
source_group("Header files\\OpenGL" FILES ${AG_HEADERS_OPENGL})
source_group("Header files\\OpenGL\\FX" FILES ${AG_HEADERS_OPENGL_FX})

include_directories(src/)

add_executable(
	autograph
	${AG_SOURCES_CORE}
	${AG_SOURCES_OPENGL}
	${AG_SOURCES_OPENGL_FX}
	${AG_HEADERS_CORE}
	${AG_HEADERS_OPENGL}
	${AG_HEADERS_OPENGL_FX}
	)

add_library(shaderpp ${SHADERPP_SOURCES})
target_link_libraries(shaderpp ${Boost_LIBRARIES})
target_include_directories(shaderpp PRIVATE ext/GSL/include)

target_link_libraries(autograph shaderpp cppformat glloadgen glfw ${Boost_LIBRARIES} ${GLFW_LIBRARIES})
target_include_directories(autograph PRIVATE ext/glm ext/GSL/include)
