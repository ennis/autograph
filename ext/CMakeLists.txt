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

# glslang

#add_subdirectory(glslang)
#set_property(TARGET glslang PROPERTY FOLDER "External")
#set_property(TARGET OGLCompilersDLL PROPERTY FOLDER "External")
#set_property(TARGET StandAlone PROPERTY FOLDER "External")
#set_property(TARGET SPIRV PROPERTY FOLDER "External")
#target_include_directories(glslang INTERFACE glfw/include)