#!/bin/bash

# Figure out the script location (to use absolute paths)
script=$(readlink -f "$0")
location=$(dirname "$script")


# Common flags
warnings="-Wall -Wextra -Wshadow -Wconversion -Wdouble-promotion -Wno-unused-function"
common="-O0 -g -D NISK_DEBUG=1 -lm"

# Source files to compile
nisk_platform_src="$location/code/linux_platform.c"
nisk_game_src="$location/code/nisk.c"
nisk_server_src="$location/code/linux_server.c"

# External headers/libraries
inc_dir="$location/external/include"
lib_dir="$location/external/lib/linux-x86_64"
sdl_lib="$lib_dir/SDL2"
sdl_flags="-I$inc_dir/SDL2 -D _REENTRANT -L$sdl_lib -lSDL2"
sdl_image_lib="$lib_dir/SDL2_image"
sdl_image_flags="-I$inc_dir/SDL2_image -D _REENTRANT -L$sdl_image_lib -lSDL2_image"
external_flags="$sdl_flags $sdl_image_flags -Wl,-rpath,$ORIGIN$sdl_lib,-rpath,$ORIGIN$sdl_image_lib"

mkdir -p build
cd build

gcc $nisk_platform_src -o nisk.out $common $warnings -ldl $external_flags
gcc $nisk_game_src -o nisk.so -shared $common
gcc $nisk_server_src -o server.out $common $warnings