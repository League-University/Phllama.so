NAME                =   phllama
INI_DIR             =   /etc/php/8.2/mods-available
EXTENSION_DIR       =   $(shell php-config --extension-dir)

COMPILER            =   g++
LINKER              =   g++

# Ollama's llama.cpp paths
OLLAMA_LLAMA_DIR    =   deps/ollama/llama/llama.cpp
OLLAMA_GGML_DIR     =   deps/ollama/ml/backend/ggml/ggml
OLLAMA_BUILD_DIR    =   build/ollama

# PHP-CPP paths
PHPCPP_DIR          =   deps/php-cpp
PHPCPP_BUILD_DIR    =   build/php-cpp

# Build output libraries
GGML_BASE_LIB       =   $(OLLAMA_BUILD_DIR)/lib/ollama/libggml-base.so
GGML_CPU_LIB        =   $(OLLAMA_BUILD_DIR)/lib/ollama/libggml-cpu-haswell.so
GGML_CUDA_LIB       =   $(OLLAMA_BUILD_DIR)/lib/ollama/libggml-cuda.so
PHPCPP_LIB          =   $(PHPCPP_DIR)/libphpcpp.a.2.4.10

COMPILER_FLAGS      =   -Wall -c -O2 -std=c++17 -fpic -o
COMPILER_FLAGS_PROD =   -Wall -c -O3 -std=c++17 -fpic -DNDEBUG -march=native -o
LINKER_FLAGS        =   -shared
LINKER_DEPENDENCIES =   $(PHPCPP_LIB) libllama.a -lstdc++fs -Lbuild/ollama/lib/ollama -lggml-base -lggml-cpu-haswell -lggml-cuda -pthread -ldl

RM                  =   rm -f
CP                  =   cp -f
MKDIR               =   mkdir -p

SOURCES             =   main.cpp ollama_interface.cpp llama_interface.cpp
OBJECTS             =   $(SOURCES:%.cpp=%.o)
PHP_CONFIG          =   php-config
PHP_CONFIG_DIRECTIVES = --includes --libs --ldflags
INCLUDES            =   $(shell $(PHP_CONFIG) --includes) -I$(OLLAMA_LLAMA_DIR)/include -I$(OLLAMA_LLAMA_DIR)/common -I$(OLLAMA_GGML_DIR)/include -Ideps/ollama/llama -I$(PHPCPP_DIR) -I$(PHPCPP_DIR)/include

all: ollama-deps php-cpp-deps $(NAME).so

# Production build with optimizations
production: clean-tests ollama-deps php-cpp-deps $(NAME)-prod.so

$(NAME)-prod.so: ${OBJECTS:.o=-prod.o} ollama-deps php-cpp-deps
	${LINKER} ${LINKER_FLAGS} -o $(NAME).so ${OBJECTS:.o=-prod.o} ${LINKER_DEPENDENCIES}

%-prod.o: %.cpp
	${COMPILER} ${COMPILER_FLAGS_PROD} $@ ${INCLUDES} $<

ollama-deps:
	$(MKDIR) $(OLLAMA_BUILD_DIR)
	cd $(OLLAMA_BUILD_DIR) && cmake ../../deps/ollama && make -j$(shell nproc)

php-cpp-deps:
	cd $(PHPCPP_DIR) && make -j$(shell nproc)

$(NAME).so: ${OBJECTS} ollama-deps php-cpp-deps
	${LINKER} ${LINKER_FLAGS} -o $@ ${OBJECTS} ${LINKER_DEPENDENCIES}

%.o: %.cpp
	${COMPILER} ${COMPILER_FLAGS} $@ ${INCLUDES} $<

install: $(NAME).so
	${CP} $(NAME).so ${EXTENSION_DIR}
	${CP} $(NAME).ini ${INI_DIR}

clean:
	${RM} *.o $(NAME).so

# Remove test files for production builds
clean-tests:
	${RM} test_*.php

clean-all: clean clean-tests
	${RM} -rf $(OLLAMA_BUILD_DIR)
	cd $(PHPCPP_DIR) && make clean

.PHONY: clean clean-all clean-tests install ollama-deps php-cpp-deps production