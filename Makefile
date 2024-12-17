.PHONY: all build clean rebuild \
		logger_build logger_clean logger_rebuild \
		stack_build stack_clean stack_rebuild	\
		clean_all clean_log clean_out clean_obj clean_deps clean_txt clean_bin \
		frontend_all frontend_build frontend_clean frontend_rebuild frontend_start \
		backend_all backend_build backend_clean backend_rebuild backend_start \
		splu_all splu_build splu_clean splu_rebuild splu_start

PROJECT_NAME = masik

BUILD_DIR = ./build
SRC_DIR = ./utils
COMPILER = gcc

DEBUG_ ?= 1

ifeq ($(origin FLAGS), undefined)

FLAGS =	-Wall -Wextra -Waggressive-loop-optimizations \
		-Wmissing-declarations -Wcast-align -Wcast-qual -Wchar-subscripts \
		-Wconversion -Wempty-body -Wfloat-equal \
		-Wformat-nonliteral -Wformat-security -Wformat-signedness -Wformat=2 -Winline -Wlogical-op \
		-Wopenmp-simd -Wpacked -Wpointer-arith -Winit-self \
		-Wredundant-decls -Wshadow -Wsign-conversion \
		-Wstrict-overflow=2 -Wsuggest-attribute=noreturn -Wsuggest-final-methods \
		-Wsuggest-final-types -Wswitch-default -Wswitch-enum -Wsync-nand \
		-Wundef -Wunreachable-code -Wunused -Wvariadic-macros \
		-Wno-missing-field-initializers -Wno-narrowing -Wno-varargs \
		-Wstack-protector -fcheck-new -fstack-protector -fstrict-overflow \
		-flto-odr-type-merging -fno-omit-frame-pointer -Wlarger-than=81920 -Wstack-usage=81920 -pie \
		-fPIE -Werror=vla \

SANITIZER = -fsanitize=address,alignment,bool,bounds,enum,float-cast-overflow,float-divide-by-zero,$\
		integer-divide-by-zero,leak,nonnull-attribute,null,object-size,return,returns-nonnull-attribute,$\
		shift,signed-integer-overflow,undefined,unreachable,vla-bound,vptr

DEBUG_FLAGS = -D _DEBUG  -ggdb -Og -g3 -D_FORTIFY_SOURCES=3 $(SANITIZER)
RELEASE_FLAGS = -DNDEBUG -O2

ifneq ($(DEBUG_),0)
FLAGS += $(DEBUG_FLAGS)
else
FLAGS += $(RELEASE_FLAGS)
endif

endif

FLAGS += $(ADD_FLAGS)


all:  libs_build frontend_all midlend_all backend_all splu_all

build: libs_build frontend_build midlend_build backend_build splu_build

start: frontend_start midlend_start backend_start splu_start

rebuild: libs_rebuild frontend_rebuild midlend_rebuild backend_rebuild splu_rebuild


frontend_all: frontend_build frontend_start

frontend_start:
	@make ADD_FLAGS="$(ADD_FLAGS)" FLAGS="$(FLAGS)" DEBUG_=$(DEBUG_) OPTS="$(FOPTS)" start -C ./frontend/

frontend_rebuild: frontend_clean frontend_build

frontend_build:
	@make ADD_FLAGS="$(ADD_FLAGS)" FLAGS="$(FLAGS)" DEBUG_=$(DEBUG_) build -C ./frontend/


midlend_clean:
	make ADD_FLAGS="$(ADD_FLAGS)" clean -C ./midlend/

midlend_all: midlend_build midlend_start

midlend_start:
	@make ADD_FLAGS="$(ADD_FLAGS)" FLAGS="$(FLAGS)" DEBUG_=$(DEBUG_) OPTS="$(MOPTS)" start -C ./midlend/

midlend_rebuild: midlend_clean midlend_build

midlend_build:
	@make ADD_FLAGS="$(ADD_FLAGS)" FLAGS="$(FLAGS)" DEBUG_=$(DEBUG_) build -C ./midlend/

midlend_clean:
	make ADD_FLAGS="$(ADD_FLAGS)" clean -C ./midlend/


backend_all: backend_build backend_start

backend_start:
	@make ADD_FLAGS="$(ADD_FLAGS)" FLAGS="$(FLAGS)" DEBUG_=$(DEBUG_) OPTS="$(BOPTS)" start -C ./backend/

backend_rebuild: backend_clean backend_build

backend_build:
	@make ADD_FLAGS="$(ADD_FLAGS)" FLAGS="$(FLAGS)" DEBUG_=$(DEBUG_) build -C ./backend/

backend_clean:
	make ADD_FLAGS="$(ADD_FLAGS)" clean -C ./backend/


splu_all: splu_build splu_start

splu_start:
	@make ADD_FLAGS="$(ADD_FLAGS)" FLAGS="$(FLAGS)" DEBUG_=$(DEBUG_) \
	AOPTS="$(SAOPTS)" POPTS="$(SPOPTS)" start -C ./libs/SPlU/

splu_rebuild: splu_clean splu_build

splu_build:
	@make ADD_FLAGS="$(ADD_FLAGS)" FLAGS="$(FLAGS)" DEBUG_=$(DEBUG_) build -C ./libs/SPlU/

splu_clean:
	make ADD_FLAGS="$(ADD_FLAGS)" clean -C ./libs/SPlU/


libs_rebuild: libs_clean libs_build

libs_build:
	@make ADD_FLAGS="$(ADD_FLAGS)" FLAGS="$(FLAGS)" DEBUG_=$(DEBUG_) build -C ./libs/stack_on_array/ && \
	 make ADD_FLAGS="$(ADD_FLAGS)" FLAGS="$(FLAGS)" DEBUG_=$(DEBUG_) build -C ./libs/logger/ && \
	 make ADD_FLAGS="$(ADD_FLAGS)" FLAGS="$(FLAGS)" DEBUG_=$(DEBUG_) build -C ./utils/

libs_clean:
	make ADD_FLAGS="$(ADD_FLAGS)" clean -C ./libs/stack_on_array/ && \
	make ADD_FLAGS="$(ADD_FLAGS)" clean -C ./libs/logger/ && \
	make ADD_FLAGS="$(ADD_FLAGS)" clean -C ./utils/


clean: libs_clean frontend_clean midlend_clean backend_clean splu_clean

clean_all:
	make ADD_FLAGS="$(ADD_FLAGS)" clean_all -C ./libs/logger         && \
	make ADD_FLAGS="$(ADD_FLAGS)" clean_all -C ./libs/stack_on_array && \
	make ADD_FLAGS="$(ADD_FLAGS)" clean_all -C ./utils/ && \
	make ADD_FLAGS="$(ADD_FLAGS)" clean_all -C ./frontend/          && \
	make ADD_FLAGS="$(ADD_FLAGS)" clean_all -C ./midlend/          && \
	make ADD_FLAGS="$(ADD_FLAGS)" clean_all -C ./libs/SPlU          && \
	make ADD_FLAGS="$(ADD_FLAGS)" clean_all -C ./backend/  