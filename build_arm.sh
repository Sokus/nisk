#!/bin/bash

# GENERAL FLAGS
#warnings="-Wall -Wextra -Wshadow -Wconversion -Wdouble-promotion -Wno-unused-function"
common="-O0 -g -D NISK_DEBUG=1 -lm"

location="$(pwd)"
nisk_platform_src="$location/code/linux_platform.c"
nisk_game_src="$location/code/nisk.c"

inc_dir="../external/include"
lib_dir="../external/lib/linux-ARM"
sdl_lib="$lib_dir/SDL2"
sdl_flags="-I$inc_dir/SDL2 -D _REENTRANT -D SDL_DISABLE_IMMINTRIN_H -L$sdl_lib -lSDL2"
sdl_image_lib="$lib_dir/SDL2_image"
sdl_image_flags="-I$inc_dir/SDL2_image -D _REENTRANT -L$sdl_image_lib -lSDL2_image"
external_flags="$sdl_flags $sdl_image_flags -Wl,-rpath,$ORIGIN$sdl_lib,-rpath,$ORIGIN$sdl_image_lib"

mkdir -p build
cd build


gcc $nisk_platform_src -o nisk.out $common $warnings -ldl $external_flags
gcc $nisk_game_src -o nisk.so -shared $common















# SERVER
# server_src="../code/linux_server.c"

# clang $server_src -o server.out $common $warnings
