

HOST		:=ubuntu@3.105.0.153 
SSH_KEY		:=~/lmeldrum_dev.pem
SSH     	:= ssh -i $(SSH_KEY) $(HOST)
DEPLOY_BR	:= deploy
RUN_MODE	:= systemd
SERVICE		:= http-server.service

define REMOTE_HEREDOC
$(SSH) bash -s <<'EOS'
set -euo

cd $(APP_DIR)

git fetch origin                               # fetch latest refs but don't merge
git checkout -B $(DEPLOY_BRANCH) origin/$(DEPLOY_BRANCH) || true
# Above: ensure a local $(DEPLOY_BRANCH) exists and tracks the remote deploy branch

git reset --hard origin/$(DEPLOY_BRANCH)
# Force the working tree to exactly match the remote deploy branch
# (discarding any accidental edits on the server)

make -j build
# -j: parallel compile

sudo systemctl restart $(SERVICE)
EOS
endef


.PHONY: deploy push-deploy remote-deploy start stop restart logs shell help


deploy: push-deploy remote-deploy 
	@echo "Deployed HEAD to $(HOST):$(APP_DIR) via branch '$(DEPLOY_BRANCH)'."

push-deploy: 
	@git rev-parse --verify HEAD >/dev/null   # sanity check: we are in a git repo
	git push origin HEAD:refs/heads/$(DEPLOY_BRANCH)
	# This creates/updates the remote 'deploy' branch to your current commit

remote-deploy:
	$(REMOTE_HEREDOC)

start: 
	$(SSH) "sudo systemctl start $(SERVICE)"

stop:
	$(SSH) "sudo systemctl stop $(SERVICE)"

restart: 
	$(SSH) "sudo systemctl restart $(SERVICE)"

logs: 
	$(SSH) "sudo journalctl -u $(SERVICE) -n 200 -f"

shell: 
	$(SSH)

help: 
	@grep -E '^[a-zA-Z_-]+:.*?## ' $(MAKEFILE_LIST) | \
		awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-18s\033[0m %s\n", $$1, $$2}'




## RUNS ON THE SERVER 

PROG	:=httpserver
CC	:=gcc
CFLAGS	:=-Wall -Iinclude -O2
LDFLAGS :=
SRC 	:=$(wildcard src/*.c)
OBJS	:=$(SRC:.c=.o)


FMT_RESET:=		tput sgr0
FMT_REDBANNER	:=tput rev; tput bold; tput setaf 1
FMT_GREENBANNER	:=tput rev; tput bold; tput setaf 2
FMT_YELLOWBANNER:=tput rev; tput bold; tput setaf 3
FMT_REV:=tput rev; tput bold; 

build: $(PROG)

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
