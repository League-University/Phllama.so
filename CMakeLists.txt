cmake_minimum_required(VERSION 3.12)
project(phllama)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find PHP
find_program(PHP_CONFIG php-config)
if(NOT PHP_CONFIG)
    message(FATAL_ERROR "php-config not found. Please install PHP development headers.")
endif()

# Get PHP configuration
execute_process(COMMAND ${PHP_CONFIG} --includes OUTPUT_VARIABLE PHP_INCLUDES OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${PHP_CONFIG} --extension-dir OUTPUT_VARIABLE PHP_EXTENSION_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${PHP_CONFIG} --version OUTPUT_VARIABLE PHP_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)

message(STATUS "Found PHP ${PHP_VERSION}")
message(STATUS "PHP includes: ${PHP_INCLUDES}")
message(STATUS "PHP extension dir: ${PHP_EXTENSION_DIR}")

# Convert includes to list format for CMake
string(REPLACE " " ";" PHP_INCLUDE_LIST ${PHP_INCLUDES})
set(PHP_INCLUDE_DIRS ${PHP_INCLUDE_LIST})

# Ollama paths (built via Makefile - CMake is for PHP-CPP only)
set(OLLAMA_LLAMA_DIR deps/ollama/llama/llama.cpp)
set(OLLAMA_GGML_DIR deps/ollama/ml/backend/ggml/ggml)

# PHP-CPP paths
set(PHPCPP_DIR deps/php-cpp)

# Build PHP-CPP library
add_subdirectory(${PHPCPP_DIR} php_cpp_build)

# Include directories
include_directories(${PHP_INCLUDE_DIRS})
include_directories(${OLLAMA_LLAMA_DIR}/include)
include_directories(${OLLAMA_LLAMA_DIR}/common)
include_directories(${OLLAMA_GGML_DIR}/include)
include_directories(deps/ollama/llama)  # For sampling_ext.h
include_directories(${PHPCPP_DIR}/include)  # PHP-CPP headers

# PHP-CPP will be built as a submodule
set(PHPCPP_LIB phpcpp)

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
    ggml
)

# Add dependency to ensure PHP-CPP is built first
add_dependencies(phllama phpcpp)

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