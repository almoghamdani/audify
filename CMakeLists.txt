cmake_minimum_required(VERSION 3.1)

# We force clang to generate code that should be compatible with macOS 10.9+ as libc++ is supported on 10.9+
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "Minimum OS X deployment version")

project(audify)

IF(DEFINED napi_build_version)
    add_definitions(-DNAPI_VERSION=${napi_build_version})
ENDIF()

IF (WIN32)
    # Change runtime library to MD
    set(CMAKE_CXX_FLAGS_RELEASE "/MD")
    set(CMAKE_CXX_FLAGS_DEBUG "/MDd")

    # Ignore all C++17 deprecation warnings on windows and unsafe warnings
    add_definitions(-D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
ENDIF()

# Use install RPATH
IF (UNIX)
    set(CMAKE_SKIP_BUILD_RPATH  FALSE)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)
    set(CMAKE_INSTALL_NAME_DIR "@rpath/")
ENDIF()

# Set RPATH as origin in Linux
IF (UNIX AND NOT APPLE)
    set(CMAKE_INSTALL_RPATH "$ORIGIN")
ENDIF()

# Use @loader_path as RPATH in macOS
IF (APPLE)
    set(CMAKE_MACOSX_RPATH TRUE)
    set(CMAKE_INSTALL_RPATH "@loader_path")
ENDIF()

# Fix issues in compiling Opus for arm32
IF (NODE_ARCH STREQUAL "arm")
    set(CMAKE_SYSTEM_PROCESSOR arm)
    set(CMAKE_C_FLAGS "-mfpu=neon")
ENDIF()

# Compile Opus
set(BUILD_SHARED_LIBS ON CACHE BOOL "")
set(BUILD_TESTING OFF CACHE BOOL "")
add_subdirectory(vendor/opus opus)
include_directories(vendor/opus/include)

# Compile RtAudio
# Enable ASIO and DirectSound support on windows
IF (WIN32)
    set(RTAUDIO_API_ASIO ON CACHE BOOL "Build ASIO API")
    set(RTAUDIO_API_DS ON CACHE BOOL "Build DirectSound API")
ENDIF ()
set(RTAUDIO_BUILD_TESTING OFF)
add_subdirectory(vendor/rtaudio rtaudio)
include_directories(vendor/rtaudio)

# Set C++20 standard
set (CMAKE_CXX_STANDARD 20)

# Get all the cpp files for audify
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")

# Add audify as a library
add_library(${PROJECT_NAME} SHARED ${SOURCES} ${CMAKE_JS_SRC})

# Remove lib prefix + add node suffix
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")

# Essential include files to build a node addon,
# You should add this line in every CMake.js based project
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_JS_INC})

# Include N-API wrappers
execute_process(COMMAND node -p "require('node-addon-api').include"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE NODE_ADDON_API_DIR
        )
string(REPLACE "\n" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
string(REPLACE "\"" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${NODE_ADDON_API_DIR})

# Compile the library
target_link_libraries(${PROJECT_NAME} ${OPENSSL_LIBS} ${CMAKE_JS_LIB} opus rtaudio)

IF (MSVC AND CMAKE_JS_NODELIB_DEF AND CMAKE_JS_NODELIB_TARGET)
  # Generate node.lib
  execute_process(COMMAND ${CMAKE_AR} /def:${CMAKE_JS_NODELIB_DEF} /out:${CMAKE_JS_NODELIB_TARGET} ${CMAKE_STATIC_LINKER_FLAGS})
ENDIF ()
