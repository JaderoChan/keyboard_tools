@echo off

cmake -B build
cmake --build build --config release -j
cmake --install build --prefix install
