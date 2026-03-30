.PHONY: all clean run debug help h ? docs 

all: 
	./setup-env.sh 

asan: 
	./setup-env.sh 

run-from-script: run


SRC_DIR	:=src
OBJ_DIR :=build

CFLAGS+= -g -fsanitize=thread -fno-omit-frame-pointer
SRC_EXT	:=.c
OBJ_EXT :=.o


SRC 	:=$(wildcard $(SRC_DIR)/*$(SRC_EXT))
OBJS 	:=$(patsubst $(SRC_DIR)/%$(SRC_EXT),$(OBJ_DIR)/%$(OBJ_EXT),$(SRC))
DEPS	:=$(patsubst $(OBJ_DIR)/%$(OBJ_EXT),$(OBJ_DIR)/%.d, $(OBJS))


$(OBJ_DIR)/%$(OBJ_EXT) : $(SRC_DIR)/%$(SRC_EXT) 
ifeq (${QUIET}, 0)
	@$(ECHO_COMP_BANNER)
	@echo
	@printf '  %s\n' $(CC)
	@printf '    %s\n' $(CFLAGS)
	@printf '    %s\n'  $@
	@printf '\n' 
endif
	@$(CC) $(CFLAGS) -c $< -o $@ 

# LINK 
$(EXE): $(EXE_DIR) $(OBJ_DIR) $(OBJS)
ifeq (${QUIET}, 0)
	@$(ECHO_LINK_BANNER)
	@echo
	@printf '  %s\n' $(CC)
	@printf '    %s\n' $(CFLAGS)
	@printf '    %s\n' $(LDFLAGS)
	@printf '    %s\n' $(OBJS)
	@printf '\n' 
	@printf '    %s\n' $(EXE)
	@printf '\n' 
	@printf '    %s\n' $(LDLIBS)
	@printf '\n' 
endif
	@$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $(EXE) $(LDLIBS)

-include $(DEPS)
# executing binary
run: $(EXE)
	@tput bold
	@printf "Build finished in "
	@printf " w/ $(NCPU) cpus...\n"
	@tput sgr0 
	@$(ECHO_EXE_BANNER)
	sudo ./$(EXE) $(ARGS)

	
#NOTE: AVOID PUTTING TARGETS IN CLEAN COMMAND! BE VERY SPECIFIC SO YOU DONT NUKE YOUR SOURCE FILES ON ACCIDENT
clean: 
	$(ECHO_CLEAN_BANNER)
	rm -f build/*.o build/**/*.o  bin/*

# ------------ DEBUGGING ------------ #
debug: $(EXE)
	$(DEBUGGER) -o run -- $(EXE) $(TERM_COLS) $(ARGS)

	
# Address san: lower overhead than thread-san, cleaner stack traces,

# ------------ EXTRAS ------------ #
$(OBJ_DIR): $(OBJ_DIR)/game
	@mkdir -p $@

$(EXE_DIR):
	@mkdir -p $@

$(OBJ_DIR)/game:
	@mkdir -p $@

help h ? docs: HELP
HELP:
	
	@$(FMT_REV)
	@printf "===== MAKE VARIABLES =====\n"; $(FMT_RESET)
	@$(FMT_ALT1) 	printf "SRC      = $(SRC)\n" 
	@$(FMT_ALT2) 	printf "EXE_DIR  = $(EXE_DIR )\n"
	@$(FMT_ALT1)	printf "EXE_NAME = $(EXE_NAME)\n"
	@$(FMT_ALT2)	printf "EXE      = $(EXE)\n"
	@$(FMT_ALT1)	printf "SRC_DIR  = $(SRC_DIR)\n"
	@$(FMT_ALT2)	printf "OBJ_DIR  = $(OBJ_DIR)\n"
	@$(FMT_ALT1)	printf "SRC_EXT  = $(SRC_EXT)\n"
	@$(FMT_ALT2)	printf "OBJ_EXT  = $(OBJ_EXT)\n"
	@$(FMT_ALT1)	printf "CC     = $(CC)\n"
	@$(FMT_ALT2)	printf "SRC      = $(SRC)\n"
	@$(FMT_ALT1)	printf "OBJS     = $(OBJS)\n"
	@$(FMT_ALT2)	printf "CFLAGS  = $(CFLAGS)\n"
	@$(FMT_ALT1)	printf "LDFLAGS  = $(LDFLAGS)\n"
	@$(FMT_ALT2)	printf "LDLIBS   = $(LDLIBS)\n"




# ------------ PRETTY PRINTING ------------ #
FMT_RESET	:=tput sgr0;
FMT_REDBANNER	:=tput rev; tput bold; tput setaf 1;
FMT_GREENBANNER	:=tput rev; tput bold; tput setaf 2;
FMT_YELLOWBANNER:=tput rev; tput bold; tput setaf 3;
FMT_REV		:=tput rev; tput bold;


FMT_ALT1	:= tput setaf 7;
FMT_ALT2	:= tput sgr0;


ECHO_LINK_BANNER := @$(FMT_YELLOWBANNER) 	printf " LINKING OBJ to BIN -> "; $(FMT_RESET) 	printf "\t"
ECHO_COMP_BANNER := @$(FMT_GREENBANNER)		printf " COMPILE SRC to OBJ -> "; $(FMT_RESET)	printf "\t"
ECHO_EXE_BANNER := @$(FMT_REDBANNER) 		printf " EXECUTING BINARY -> "; $(FMT_RESET)	printf "\t"
ECHO_CLEAN_BANNER:= @$(FMT_REV) 		printf " REMOVING EXEUTABLES AND OBJECT FILES... "; $(FMT_RESET)printf "\t"


# EXTRA MAKEFILE STUFF:
# :_______1-1_______: To exclude certain files from compilation:
# EXCLUDE_PAT	:= unit_tests.c
# SRC 		:= $(filter-out src/$(EXCLUDE_SRC), $(wildcard src/*.c))
