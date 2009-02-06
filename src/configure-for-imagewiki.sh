#!/bin/sh
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig
make clean
autoconf
./configure --bindir=/www/sites/imagewiki/sift/bin --libdir=/www/sites/imagewiki/sift/lib
