# cppformat
file(GLOB CPPFORMAT_SOURCES cppformat/*.cc)
file(GLOB CPPFORMAT_HEADERS cppformat/*.h)
add_library(cppformat STATIC ${CPPFORMAT_SOURCES} ${CPPFORMAT_HEADERS})
set_property(TARGET cppformat PROPERTY FOLDER "External")
target_include_directories(cppformat INTERFACE cppformat)

# glloadgen
add_subdirectory(glloadgen)

# GLFW 
option(BUILD_SHARED_LIBS "..." OFF)
option(GLFW_BUILD_EXAMPLES "..." OFF)
option(GLFW_BUILD_TESTS "..." OFF)
option(GLFW_BUILD_DOCS "..." OFF)
option(GLFW_INSTALL "..." OFF)
option(GLFW_DOCUMENT_INTERNALS "..." OFF)
add_subdirectory(glfw)
set_property(TARGET glfw PROPERTY FOLDER "External")
target_include_directories(glfw INTERFACE glfw/include)

# assimp
option(BUILD_SHARED_LIBS "..." OFF)
option(ASSIMP_BUILD_ASSIMP_TOOLS "..." OFF)
option(ASSIMP_BUILD_SAMPLES "..." OFF)
option(ASSIMP_BUILD_TESTS "..." OFF)
add_subdirectory(assimp)
set_property(TARGET assimp PROPERTY FOLDER "External")
#set_property(TARGET zlibstatic PROPERTY FOLDER "External")
#set_property(TARGET UpdateAssimpLibsDebugSymbolsAndDLLs PROPERTY FOLDER "External")
#set_property(TARGET uninstall PROPERTY FOLDER "External")
target_include_directories(assimp INTERFACE assimp/include)

# docopt
option(WITH_TESTS           "Build tests."       OFF)
option(WITH_EXAMPLE         "Build example."     OFF)
option(WITH_STATIC          "Build static libs." ON)
add_subdirectory(docopt.cpp)
target_include_directories(docopt INTERFACE docopt.cpp)
target_include_directories(docopt_s INTERFACE docopt.cpp)
set_property(TARGET docopt PROPERTY FOLDER "External")
set_property(TARGET docopt_s PROPERTY FOLDER "External")

# filesystem
add_library(filesystem INTERFACE)
target_include_directories(filesystem INTERFACE filesystem)

# stb
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE stb)

# RxCpp
#add_subdirectory(RxCpp)
add_library(rxcpp INTERFACE)
target_include_directories(rxcpp INTERFACE RxCpp/Rx/v2/src)

# vulkan
if (VULKAN_FOUND)
add_library(vulkan INTERFACE)
target_include_directories(vulkan INTERFACE ${VULKAN_INCLUDE_DIR})
target_link_libraries(vulkan INTERFACE ${VULKAN_LIBRARY})
endif()

# vkcpp
add_library(vkcpp INTERFACE)
target_include_directories(vkcpp INTERFACE vkcpp)

# variant 
add_library(variant INTERFACE)
target_include_directories(variant INTERFACE variant/include)

# glslang

#add_subdirectory(glslang)
#set_property(TARGET glslang PROPERTY FOLDER "External")
#set_property(TARGET OGLCompilersDLL PROPERTY FOLDER "External")
#set_property(TARGET StandAlone PROPERTY FOLDER "External")
#set_property(TARGET SPIRV PROPERTY FOLDER "External")
#target_include_directories(glslang INTERFACE glfw/include)
