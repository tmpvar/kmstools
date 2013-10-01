#!/usr/bin/bash
FLAGS="-g -Wextra"

clang $FLAGS -o out/list-modes list-modes.c `pkg-config --cflags --libs libdrm`

clang $FLAGS -o out/list-devices list-devices.c -ludev


clang $FLAGS -o out/eglkms eglkms.c `pkg-config --cflags --libs libdrm egl gl gbm`
clang $FLAGS -o out/eglkms-mouse eglkms-mouse.c `pkg-config --cflags --libs libdrm egl gl gbm`
