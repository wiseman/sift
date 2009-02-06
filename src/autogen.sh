#!/bin/sh

aclocal
libtoolize --automake
autoconf
automake
