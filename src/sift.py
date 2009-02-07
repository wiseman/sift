#!/usr/bin/python

import ctypes
import sys
import _sift

siftlib = ctypes.pydll.LoadLibrary('_siftmodule.so')
libc = ctypes.pydll.LoadLibrary("libc.so.6")


class sift_ptr(ctypes.c_void_p):
  def __del__(self):
    if bool(self): siftlib.remove_ref(self)



class SIFTError(Exception):
  def __init__(self, msg, errno):
      Exception.__init__(self, msg)
      self.errno = errno
      sys.stderr.write('%s\n' % (self,))
  def __str__(self):
      if self.errno != 0:
          return "S! %s: %s" % (self.message, libc.strerror(self.errno))
      else:
          return "S! %s" % (self.message,)

def make_sift_error(msg, errno):
  return SIFTError(msg, errno)


class MatchResult:
    def __init__(self, tuple):
        self.label = tuple[0]
        self.score = tuple[1]
        self.percentage = tuple[2]

    def __str__(self):
        return "<MatchResult %s %s %s%%>" % (self.label, self.score, self.percentage)
    def __repr__(self):
        return "<MatchResult %s %s %s%%>" % (self.label, self.score, self.percentage)

                       



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

    def save(self, path):
      return siftlib.sift_database_save(self, path)

    def load(self, path):
      return siftlib.sift_database_load(self, path)




siftlib.sift_database_new.argtypes = []
siftlib.sift_database_new.restype = ctypes.c_void_p
siftlib.sift_database_add_image_file.argtypes = [Database, ctypes.c_char_p, ctypes.c_char_p]
siftlib.sift_database_contains_label.argtypes = [Database, ctypes.c_char_p]
siftlib.sift_database_image_count.argtypes = [Database]
siftlib.sift_database_feature_count.argtypes = [Database]
siftlib.sift_database_remove_image.argtypes = [Database, ctypes.c_char_p]
siftlib.sift_database_save.argtypes = [Database, ctypes.c_char_p]
siftlib.sift_database_load.argtypes = [Database, ctypes.c_char_p]

libc.strerror.argtypes = [ctypes.c_int]
libc.strerror.restype = ctypes.c_char_p;
