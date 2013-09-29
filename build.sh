#!/usr/bin/bash

clang -Wextra -o out/list-modes list-modes.c `pkg-config --cflags --libs libdrm`

