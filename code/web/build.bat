@echo off
IF NOT EXIST ..\..\build mkdir ..\..\build
IF NOT EXIST ..\..\build\web mkdir ..\..\build\web

call tsc

pushd ..\..\build\web

call clang --target=wasm32 -nostdlib -fvisibility=hidden -Wl,--no-entry -Wl,--strip-all -Wl,--export-dynamic -Wl,--import-memory -Wl,--allow-undefined-file=..\..\code\web\wasm.syms ..\..\code\web\web_platform.c -o gng.wasm 
xcopy /y /s ..\..\code\web\index.html index.html*

popd
