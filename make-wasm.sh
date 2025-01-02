#!/usr/bin/env bash
set -e

mkdir -p build-wasm 
cp res/icons/ChillyGB.svg build-wasm/icon.svg 
emcc -o build-wasm/index.html \
	src/main.c src/modules/* third-party/cJSON/cJSON.c\
	-Os -Wall third-party/raylib/src/libraylib.a \
	-I. -Ithird-party/raylib/src/ -L. -Lthird-party/raylib/src/ -s USE_GLFW=3 \
	-Ithird-party/raylib-nuklear/include \
	-Ithird-party/cJSON \
	-DPLATFORM_WEB \
	-sEXPORTED_RUNTIME_METHODS=ccall \
	-sEXPORTED_FUNCTIONS=_load_settings,_pause_game,_load_cartridge_wasm,_main,_in_game,_cameraCallback \
	--shell-file=wasm-frontend/shell.html \
	-lidbfs.js \
	--preload-file ./res 

cp -r wasm-frontend/* build-wasm/ 
rm build-wasm/shell.html 
# Add build date to service worker, so it will update on every build
echo "// Built on $(date) - HEAD:$(git rev-parse HEAD)" >> build-wasm/serviceWorker.js