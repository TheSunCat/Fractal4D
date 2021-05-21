cmake_minimum_required(VERSION 3.15.0 FATAL_ERROR)

project(Fractal4D C CXX)

#include(CMake/Utils.cmake)
#include(CMake/GlobalSettingsInclude.cmake OPTIONAL)
#set_property(GLOBAL PROPERTY USE_FOLDERS ON)

include_directories(include)

set(PROJECT_NAME Fractal4D)

################################################################################
# Source groups
################################################################################
set(Header_Files
    "Constants.h"
    "Shader.h"
    "Util.h"
)
source_group("Header Files" FILES ${Header_Files})

set(Resource_Files
    "res/raytrace.comp"
    "res/screen.frag"
    "res/screen.vert"
)
source_group("Resource Files" FILES ${Resource_Files})

set(Source_Files
    "glad.c"
    "Fractal4D.cpp"
    "Shader.cpp"
    "Util.cpp"
)
source_group("Source Files" FILES ${Source_Files})

set(ALL_FILES
    ${Header_Files}
    ${Resource_Files}
    ${Source_Files}
)


add_executable(${PROJECT_NAME} ${ALL_FILES})
set(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

################################################################################
# Dependencies
################################################################################
if(UNIX)
set(ADDITIONAL_LIBRARY_DEPENDENCIES
    "glfw"
    "dl"
)
else()
set(ADDITIONAL_LIBRARY_DEPENDENCIES
    "glfw"
)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE "${ADDITIONAL_LIBRARY_DEPENDENCIES}")


link_directories(${PROJECT_NAME} PRIVATE
    "lib"
)

