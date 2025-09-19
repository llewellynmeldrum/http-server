# -------- build rules (used on the SERVER) --------
PROG	:= httpserver
CC	:= gcc
CFLAGS	:= -Wall -Iinclude -O2
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
run-local: $(PROG)
	@$(FMT_REDBANNER)
	@echo " EXECUTING BINARY (LOCALHOST BUILD): "
	@$(FMT_RESET)
	./$(PROG) -l $(A)
	@printf "\n"

debug: $(PROG)
	lldb -o run -- $(PROG) $(A)

clean:
	@$(FMT_REV)
	@echo " REMOVING EXECUTABLES AND OBJECT FILES... "
	@$(FMT_RESET)
	rm -f $(PROG) $(OBJS)

# REMOTE STUFF
REMOTE_DIR := /home/ubuntu/http-server
TMUX_SESSION    := http-server-tmux 
REMOTE_BIN := ./httpserver 

# SSH 
HOST       := ubuntu@3.105.0.153
SSH_KEY    := /Users/llewie/lmeldrum_dev.pem
SSH := ssh -i $(SSH_KEY) $(HOST)

RSYNC_CONF  := rsync -az -e "ssh -i $(SSH_KEY) -o IdentitiesOnly=yes" \
        --delete --exclude '.git' --exclude 'build/' --exclude '*.o'
.PHONY: deploy sync run-remote stop logs shell

# One-liner you’ll use most: copy → build on server → (re)start in tmux
deploy: sync run-remote

# Copy your working tree to the server (fast: only deltas)
sync:
	$(RSYNC_CONF) ./ $(HOST):$(REMOTE_DIR)/

# Build and (re)start ON THE SERVER (never builds locally)
run-remote:
	$(SSH) "set -e; cd $(REMOTE_DIR); \
		make -j build; \
		tmux kill-session -t $(TMUX_SESSION) 2>/dev/null || true; \
		tmux new -d -s $(TMUX_SESSION) 'cd $(REMOTE_DIR) && sudo $(REMOTE_BIN) $(A) 2>server.err.log'; \
		echo 'running in tmux: $(TMUX_SESSION). stderr -> server.err.log'"
	$(SSH) "tail -f $(REMOTE_DIR)/server.err.log"

# Stop the server cleanly
stop:
	$(SSH) "tmux kill-session -t $(TMUX_SESSION) 2>/dev/null || pkill -f '$(REMOTE_BIN)' || true; echo ' stopped'"

# Quick peek at output (for live tail: `make shell` then `tmux attach -t app`)
logs:
	$(SSH) "tail -f $(REMOTE_DIR)/server.err.log"

# Jump into the box
shell:
	$(SSH)


# Perhaps change te commands to scripts?
# Would like to see stderr directly.
