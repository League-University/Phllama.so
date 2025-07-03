cmake_minimum_required(VERSION 3.12)
project(phllama)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find PHP
find_package(PkgConfig REQUIRED)
pkg_check_modules(PHP REQUIRED php)

# Add llama.cpp subdirectory
add_subdirectory(deps/llama.cpp)

# Include directories
include_directories(${PHP_INCLUDE_DIRS})
include_directories(deps/llama.cpp/include)
include_directories(deps/llama.cpp/common)

# Find PHP-CPP
find_library(PHPCPP_LIB phpcpp PATHS /usr/local/lib /usr/lib)
if(NOT PHPCPP_LIB)
    message(FATAL_ERROR "PHP-CPP library not found")
endif()

# Source files
set(SOURCES
    main.cpp
    ollama_interface.cpp
    llama_interface.cpp
)

# Create shared library
add_library(phllama SHARED ${SOURCES})

# Link libraries
target_link_libraries(phllama 
    ${PHPCPP_LIB}
    llama
    common
)

# Set properties
set_target_properties(phllama PROPERTIES
    PREFIX ""
    SUFFIX ".so"
    POSITION_INDEPENDENT_CODE ON
)

# Install target
install(TARGETS phllama
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)