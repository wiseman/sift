CLEANFILES = *.pyc

AM_CFLAGS=`pkg-config --cflags opencv` `pkg-config --cflags gtk+-2.0` `pkg-config --cflags gsl` -I /usr/include/python2.5 -Wall
AM_CXXFLAGS=`pkg-config --cflags opencv` `pkg-config --cflags gtk+-2.0` `pkg-config --cflags gsl` -I /usr/include/python2.5
AM_LDFLAGS=`pkg-config --libs opencv` `pkg-config --libs gtk+-2.0` `pkg-config --libs gsl`


lib_LTLIBRARIES=libsift.la _siftmodule.la
libsift_la_SOURCES=imgfeatures.c kdtree.c minpq.c sift.c utils.c xform.c SIFT.cpp


bin_PROGRAMS=sift-add sift-del sift-list sift-match sift-convert dspfeat siftfeat

sift_add_SOURCES=sift-add.cpp
sift_add_LDADD=libsift.la

sift_del_SOURCES=sift-del.cpp
sift_del_LDADD=libsift.la

sift_list_SOURCES=sift-list.cpp
sift_list_LDADD=libsift.la

sift_convert_SOURCES=sift-convert.cpp
sift_convert_LDADD=libsift.la

sift_match_SOURCES=sift-match.cpp
sift_match_LDADD=libsift.la


dspfeat_SOURCES=dspfeat.c
dspfeat_LDADD=libsift.la

siftfeat_SOURCES=siftfeat.c
siftfeat_LDADD=libsift.la

_siftmodule_la_SOURCES = siftmodule.cpp
_siftmodule_la_LDFLAGS = -module
_siftmodule_la_LIBADD = libsift.la
