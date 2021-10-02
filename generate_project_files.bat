@echo off
echo Generating project files in .\build ...

if not exist "\build\" md build

pushd .\build

cmake ..\ -DCMAKE_GENERATOR_PLATFORM=Win32

popd
