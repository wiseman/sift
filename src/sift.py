#!/usr/bin/python

import ctypes

import _sift

siftlib = ctypes.pydll.LoadLibrary('_siftmodule.so')


class sift_ptr(ctypes.c_void_p):
  def __del__(self):
    if bool(self): siftlib.remove_ref(self)



class SIFTError(Exception):
  def __init__(self, msg):
      Exception.__init__(self, msg)
  def __str__(self):
      return "%s" % (self.message,)

def make_sift_error(msg):
  return SIFTError(msg)


class MatchResult:
  def __init__(self, tuple):
    self.label = tuple[0]
    self.score = tuple[1]
    self.percentage = tuple[2]

                       



class Database(sift_ptr):
    def __init__(self, file=None):
        self.value = siftlib.sift_database_new()
        
    def add_image_file(self, path, label=None):
        if not label: label = path
        siftlib.sift_database_add_image_file(self, path, label)

    def contains_label(self, label):
        return siftlib.sift_database_contains_label(self, label)

    def remove_image(self, label):
        return siftlib.sift_database_remove_image(self, label)

    def image_count(self):
        return siftlib.sift_database_image_count(self)

    def feature_count(self):
        return siftlib.sift_database_feature_count(self)

    def match_image_file(self, path):
        return [MatchResult(t) for t in _sift.database_match_image_file(self, path)]



siftlib.sift_database_new.argtypes = []
siftlib.sift_database_new.restype = ctypes.c_void_p
siftlib.sift_database_add_image_file.argtypes = [Database, ctypes.c_char_p, ctypes.c_char_p]
siftlib.sift_database_contains_label.argtypes = [Database, ctypes.c_char_p]
siftlib.sift_database_image_count.argtypes = [Database]
siftlib.sift_database_feature_count.argtypes = [Database]
siftlib.sift_database_remove_image.argtypes = [Database, ctypes.c_char_p]
