#!/bin/sh
rm -rf binary
mkdir binary
cd code
./configure -flto || exit 1
make || exit 1
install -m 755 -s lingeling ../binary
