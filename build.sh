#!/usr/bin/env bash
set -eu
clang generate.c -o generate -std=gnu2x -g
./generate
rm -rf public || true
mkdir public
cp -r pdf public/pdf
cp index.html public/index.html
