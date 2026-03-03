cmake_minimum_required(VERSION 3.12)

project(funkyactors VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

message(STATUS "Configuring funkyactors library")
message(STATUS "C++ Standard: C++${CMAKE_CXX_STANDARD}")

# Fetch standalone Asio via FetchContent
include(FetchContent)
message(STATUS "Downloading standalone Asio via FetchContent")
FetchContent_Declare(
  asio
  GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
  GIT_TAG asio-1-30-2
)
FetchContent_MakeAvailable(asio)

# Create an interface library for asio (same pattern as funkysnakes)
add_library(asio INTERFACE)
target_include_directories(asio INTERFACE
  $<BUILD_INTERFACE:${asio_SOURCE_DIR}/asio/include>
)
target_compile_definitions(asio INTERFACE ASIO_STANDALONE ASIO_NO_DEPRECATED)

# Link with pthread on Unix systems
if(UNIX)
  find_package(Threads REQUIRED)
  target_link_libraries(asio INTERFACE Threads::Threads)
endif()

# Funkyactors library (mostly header-only with one implementation file)
add_library(funkyactors
  src/timer/asio_timer_core.cpp
)

target_include_directories(funkyactors
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${asio_SOURCE_DIR}/asio/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_definitions(funkyactors
  PUBLIC
    ASIO_STANDALONE
    ASIO_NO_DEPRECATED
)

# Link pthread on Unix (needed by asio)
if(UNIX)
  target_link_libraries(funkyactors PUBLIC Threads::Threads)
endif()

target_compile_options(funkyactors PRIVATE
  -Wall -Wextra -Wpedantic -Werror
  $<$<CONFIG:Debug>:-g -O0>
  $<$<CONFIG:Release>:-O3>
)

# Install rules (for use as external library)
install(TARGETS funkyactors
  EXPORT funkyactors-targets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
)

# Install funkyactors headers
install(DIRECTORY include/
  DESTINATION include
)

# Install asio headers (bundled with funkyactors for consumers)
install(DIRECTORY ${asio_SOURCE_DIR}/asio/include/
  DESTINATION include
  FILES_MATCHING PATTERN "*.hpp" PATTERN "*.ipp"
)

install(EXPORT funkyactors-targets
  FILE funkyactors-targets.cmake
  NAMESPACE funkyactors::
  DESTINATION lib/cmake/funkyactors
)

# Create config file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/funkyactors-config-version.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/funkyactors-config.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/funkyactors-config.cmake"
  @ONLY
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/funkyactors-config.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/funkyactors-config-version.cmake"
  DESTINATION lib/cmake/funkyactors
)
