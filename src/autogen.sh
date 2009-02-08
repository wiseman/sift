#!/bin/sh

set -ex

autoreconf -f -i -Wall
rm -rf autom4te.cache config.h.in~
