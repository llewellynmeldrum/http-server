#!/bin/bash

export QUIET=1
export NCPU=1
export CC=clang
export OS=`uname -s`

if [[ $OS == 'Darwin' ]]; then
    # macos
    NCPU=`sysctl -n hw.ncpu`
    CC=/opt/homebrew/opt/llvm/bin/clang
elif [[ $OS == 'Linux' ]]; then
    NCPU=`nproc`
fi

if [[ $1 == '-q' ]]; then
	echo "Silencing build commands (-q)"
    QUIET=1
fi

export CFLAGS
CFLAGS=" -std=c23 "
CFLAGS+=" -Wall -Wimplicit-fallthrough -Werror -Wno-unused "
CFLAGS+=" -Iinclude"
CFLAGS+=" -MMD -MP "
CFLAGS+=" -fno-show-column"
CFLAGS+=" -fno-diagnostics-show-option"
CFLAGS+=" -fdiagnostics-fixit-info"
CFLAGS+=" `pkg-config sdl3 --cflags`"

export EXE_DIR="bin"
export EXE_NAME="cstris"
export EXE="$EXE_DIR/$EXE_NAME"

shift 1
bear -- make run-from-script -j$NCPU $@ 

