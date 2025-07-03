NAME                =   phllama
INI_DIR             =   /etc/php/8.2/mods-available
EXTENSION_DIR       =   $(shell php-config --extension-dir)

COMPILER            =   g++
LINKER              =   g++

LLAMA_DIR           =   deps/llama.cpp
LLAMA_LIB           =   $(LLAMA_DIR)/build/src/libllama.a
LLAMA_COMMON_LIB    =   $(LLAMA_DIR)/build/common/libcommon.a

COMPILER_FLAGS      =   -Wall -c -O2 -std=c++17 -fpic -o
LINKER_FLAGS        =   -shared
LINKER_DEPENDENCIES =   -lphpcpp -lstdc++fs $(LLAMA_LIB) $(LLAMA_COMMON_LIB) -pthread

RM                  =   rm -f
CP                  =   cp -f
MKDIR               =   mkdir -p

SOURCES             =   main.cpp ollama_interface.cpp llama_interface.cpp
OBJECTS             =   $(SOURCES:%.cpp=%.o)
PHP_CONFIG          =   php-config
PHP_CONFIG_DIRECTIVES = --includes --libs --ldflags
INCLUDES            =   $(shell $(PHP_CONFIG) --includes) -I$(LLAMA_DIR)/include -I$(LLAMA_DIR)/common

all: llama-deps $(NAME).so

llama-deps:
	cd $(LLAMA_DIR) && mkdir -p build && cd build && cmake .. && make -j$(shell nproc)

$(NAME).so: ${OBJECTS} llama-deps
	${LINKER} ${LINKER_FLAGS} -o $@ ${OBJECTS} ${LINKER_DEPENDENCIES}

%.o: %.cpp
	${COMPILER} ${COMPILER_FLAGS} $@ ${INCLUDES} $<

install: $(NAME).so
	${CP} $(NAME).so ${EXTENSION_DIR}
	${CP} $(NAME).ini ${INI_DIR}

clean:
	${RM} *.o $(NAME).so

.PHONY: clean install