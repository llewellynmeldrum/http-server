# -------- build rules (used on the SERVER) --------
PROG	:= httpserver
CC	:= gcc
CFLAGS	:= -Wall -Iinclude -O2 -g3
LDFLAGS :=
SRC 	:= $(wildcard src/*.c)
OBJS	:= $(SRC:.c=.o)

FMT_RESET          := tput sgr0 2>/dev/null || true
FMT_REDBANNER      := tput rev; tput bold; tput setaf 1 2>/dev/null || true
FMT_GREENBANNER    := tput rev; tput bold; tput setaf 2 2>/dev/null || true
FMT_YELLOWBANNER   := tput rev; tput bold; tput setaf 3 2>/dev/null || true
FMT_REV            := tput rev; tput bold 2>/dev/null || true

build: $(PROG)

# compile .c into .o (compilation proper)
%.o : %.c
	@$(FMT_GREENBANNER)
	@echo " COMPILE SRC -> OBJ  > "
	@$(FMT_RESET)
	$(CC) $(CFLAGS) -c $< -o $@
	@printf "\n"

# link to final executable
$(PROG): $(OBJS)
	@$(FMT_YELLOWBANNER)
	@echo " LINKING OBJ -> BIN  > "
	@$(FMT_RESET)
	$(CC) $(LDFLAGS) $(OBJS) -o $(PROG) $(LDLIBS)
	@printf "\n"

# run locally (handy if you ever do a local build)
run: $(PROG)
	@$(FMT_REDBANNER)
	@echo " EXECUTING BINARY (LOCALHOST BUILD): "
	@$(FMT_RESET)
	./$(PROG) $(A)
	@printf "\n"

debug: $(PROG)
	lldb -o run -- $(PROG) -l $(A)

clean:
	@$(FMT_REV)
	@echo " REMOVING EXECUTABLES AND OBJECT FILES... "
	@$(FMT_RESET)
	rm -f $(PROG) $(OBJS)

