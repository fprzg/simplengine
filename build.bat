@echo off

cls

IF NOT EXIST BUILD mkdir BUILD

pushd BUILD

clang -Wno-writable-strings -Wno-deprecated-declarations -Wno-null-dereference ..\code\simplengine.cpp -o simplengine.exe -lgdi32 -lopengl32 -lShlwapi -lkernel32 -lxinput -luser32 -g -gcodeview

popd