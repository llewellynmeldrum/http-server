.PHONY: $(PROG) all clean run debug

# I use some custom banner commands
SHELL	:= /bin/zsh

PROG	:=httpserver
CC	:=gcc
CFLAGS	:=-Wall -Iinclude -g 
LDFLAGS :=
SRC 	:=$(wildcard src/*.c)
OBJS	:=$(SRC:.c=.o)

ifndef PROG
	@$(FMT_REDBANNER)
	@echo "SET PROG TO SOMETHING!\n"
	@$(FMT_RESET)
endif


# external deps
LDFLAGS	+=
CFLAGS	+=
LDLIBS  +=




# TERM_COLS:=$(shell tput cols) 
FMT_RESET:=tput sgr0
FMT_REDBANNER	:=tput rev; tput bold; tput setaf 1
FMT_GREENBANNER	:=tput rev; tput bold; tput setaf 2
FMT_YELLOWBANNER:=tput rev; tput bold; tput setaf 3
FMT_REV:=tput rev; tput bold; 
all: $(PROG)

#compile .c into .o (compilation proper)
%.o : %.c
	@$(FMT_GREENBANNER)
	@echo " COMPILE SRC -> OBJ  > " 
	@$(FMT_RESET)
	$(CC) $(CFLAGS) -c $< -o $@
	@printf "\n"

# build executable (linking)
$(PROG): $(OBJS) 
	@$(FMT_YELLOWBANNER)
	@echo " LINKING OBJ -> BIN  > "
	@$(FMT_RESET)
	$(CC) $(LDFLAGS) $(OBJS) -o $(PROG) $(LDLIBS)
	@printf "\n"

# RUN PROGRAM - DEFINE A:= ANY PROGRAM_ARGS
run: $(PROG)
	@$(FMT_REDBANNER)
	@echo " EXECUTING BINARY: " 
	@$(FMT_RESET)
	./$(PROG) $(A)
	@printf "\n"

# DEBUG PROGRAM
debug: $(PROG)
	lldb -o run -- $(PROG) $(TERM_COLS) $(A)
clean: 
	@$(FMT_REV)
	@echo " REMOVING EXECUTABLES AND OBJECT FILES... "
	@$(FMT_RESET)
	rm -f $(PROG) $(OBJS)
