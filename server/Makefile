.PHONY: all build test test-build clean
# ---
#MAKEFLAGS   +=-j8

CC			:=clang
CPPFLAGS    :=-Iinclude
CFLAGS      :=-std=gnu23
CFLAGS      +=-Wall -Werror
CFLAGS      +=-Wimplicit-fallthrough
CFLAGS      +=-Wno-unused
CFLAGS      +=-MMD -MP
CFLAGS      +=-fno-show-column
CFLAGS      +=-fno-diagnostics-show-option
CFLAGS      +=-fdiagnostics-fixit-info

LDFLAGS     := 
LDLIBS      := 

# ---
APP_SRC     :=$(shell find ./src -type f -name '*.c')
APP_OBJ     :=$(patsubst ./src/%.c,./build/%.o,$(APP_SRC))
APP_DEPS    :=$(APP_OBJ:.o=.d)
APP_EXE     :=./bin/server

TEST_SRC    :=$(shell find ./test -type f -name '*.c')
TEST_OBJ    :=$(patsubst ./test/%.c,./build/test/%.o,$(TEST_SRC))
TEST_DEPS   :=$(TEST_OBJ:.o=.d)
TEST_EXES   := $(patsubst ./build/test/%.o,./bin/test/%,$(TEST_OBJ))
TEST_CPPFLAGS :=-Itest

CORE_OBJ    :=$(filter-out ./build/main.o,$(APP_OBJ))
# ---

all: run


test-build: $(TEST_EXES) 

test: $(TEST_EXES)
	@for t in $^; do \
	echo "Running $$t"; \
		$$t || exit 1; \
	done

build: $(APP_EXE)
run: $(APP_EXE)
	#sudo ./$(APP_EXE) $(ARGS)
	./$(APP_EXE) $(ARGS)

# NOTE: Link APP (build main executable)
$(APP_EXE): $(APP_OBJ)
	@mkdir -p $(dir $@)
	@$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

# NOTE: Link TESTS (build each executable)
$(TEST_EXES): ./bin/test/%: ./build/test/%.o $(CORE_OBJ)
	@mkdir -p $(dir $@)
	@$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

# ---

# NOTE: Compile APP
./build/%.o : ./src/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
    
# NOTE: Compile TESTS
./build/test/%.o : ./test/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(CPPFLAGS) $(TEST_CPPFLAGS) $(CFLAGS) -c $< -o $@

# ---

-include $(APP_DEPS) $(TEST_DEPS)

asan: ASAN_ENV=ASAN_OPTIONS=abort_on_error=0
asan: CFLAGS+=-fsanitize=address -fno-omit-frame-pointer -g
asan: LDFLAGS+=-fsanitize=address -fno-omit-frame-pointer -g
asan:clean  all

tsan: TSAN_ENV=TSAN_OPTIONS=abort_on_error=0
tsan: CFLAGS+=-fsanitize=thread -fno-omit-frame-pointer -g
tsan: LDFLAGS+=-fsanitize=thread -fno-omit-frame-pointer -g
tsan: all

# ---
clean: 
	rm -rf build bin

# ---
debug: $(APP_EXE)
	$(DEBUGGER) -o run -- $(APP_EXE) $(ARGS)

# ---
UNAME:= $(shell uname)
ifeq ($(UNAME), Darwin)
	DEBUGGER :=lldb
else ifeq ($(UNAME), Linux)
	DEBUGGER :=gdb
else
	$(error Unsupported platform.)
endif
