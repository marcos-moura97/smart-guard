#!/bin/bash

# Configure
mkdir -p build
cmake -S . -B ./build

# Build
cmake --build ./build --config Debug --target all --

# Merge compile_commands.json
if ls ./build/**/compile_commands.json &> /dev/null; then
    jq -s add ./build/**/compile_commands.json > ./build/compile_commands.json
fi