#!/bin/sh

mkdir build-wasm
cp res/icons/ChillyGB.svg build-wasm/icon.svg
emcc -o build-wasm/index.html \
	src/main.c src/modules/* cJSON/cJSON.c\
	-Os -Wall raylib/src/libraylib.a \
	-I. -Iraylib/src/ -L. -Lraylib/src/ -s USE_GLFW=3 \
	-DPLATFORM_WEB \
	-sEXPORTED_RUNTIME_METHODS=ccall \
	-sEXPORTED_FUNCTIONS=_load_settings,_pause_game,_load_cartridge,_main,_in_game \
	--shell-file=wasm-frontend/shell.html \
	-lidbfs.js \
	--preload-file ./res