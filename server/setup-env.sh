#!/bin/bash

export QUIET=0
export NCPU=1
export CC=clang
export OS=`uname -s`
export CFLAGS="-fsanitize=address -fno-omit-frame-pointer -g"
export ASAN_ENV=ASAN_OPTIONS=abort_on_error=0
CLEANUP_FIRST=1
export DEBUGGER='NO_DEBUGGER'

if [[ $OS == 'Darwin' ]]; then
    # macos
    NCPU=`sysctl -n hw.ncpu`
    DEBUGGER=lldb
    CC=/opt/homebrew/opt/llvm/bin/clang
elif [[ $OS == 'Linux' ]]; then
    NCPU=`nproc`
    DEBUGGER=gdb
fi

if [[ $1 == '-q' ]]; then
	echo "Silencing build commands (-q)"
    QUIET=1
fi

CFLAGS=" -std=gnu23 "
CFLAGS+=" -Wall -Wimplicit-fallthrough -Werror -Wno-unused "
CFLAGS+=" -Iinclude"
CFLAGS+=" -MMD -MP "
CFLAGS+=" -fno-show-column"
CFLAGS+=" -fno-diagnostics-show-option"
CFLAGS+=" -fdiagnostics-fixit-info"
CFLAGS+=" `pkg-config sdl3 --cflags`"

export EXE_DIR="bin"
export EXE_NAME="server"
export EXE="$EXE_DIR/$EXE_NAME"

shift 1
if [[ $CLEANUP_FIRST -eq 1 ]]; then
    make clean
fi
make run-from-script -j$NCPU $@ 

