#!/usr/bin/env bash
set -eux
OWD=$PWD
INTERMEDIATE_DIR=$OWD/intermediate
export WEB_PUBLIC_DIRECTORY=$OWD/public
rm -rf $INTERMEDIATE_DIR || true
rm -rf $WEB_PUBLIC_DIRECTORY || true
mkdir -p $INTERMEDIATE_DIR
clang src/generate.c -o $INTERMEDIATE_DIR/generate -std=gnu2x -g
cd src/resume
pdflatex resume.tex
mkdir -p $WEB_PUBLIC_DIRECTORY/resume
cp resume.pdf $WEB_PUBLIC_DIRECTORY/resume
cd $OWD
$INTERMEDIATE_DIR/generate
