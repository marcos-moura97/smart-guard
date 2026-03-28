#!/bin/bash

# Configure
mkdir -p ./build/test
cmake -S test -B ./build/test

# Build
cmake --build ./build/test --config Debug --target all --

# Merge compile_commands.json
if ls ./build/**/compile_commands.json &> /dev/null; then
    jq -s add ./build/**/compile_commands.json > ./build/compile_commands.json
fi

# Run tests
ctest --test-dir ./build/test -C Debug --output-on-failure