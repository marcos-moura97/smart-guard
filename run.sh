#!/bin/bash

# Build
./build.sh

# Flash
picotool load ./build/smart_guard.elf -fx
