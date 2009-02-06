#include <Python.h>
#include <iostream>

#include "SIFT.hpp"
#include "sift.h"


PyObject* siftdb_t;


void* get_c_ptr(PyObject *c_void_p) {
  union {
    const void * buffer;
    void * const * c_ptr;
  } p;
  Py_ssize_t buflen;
  if (PyObject_AsReadBuffer(c_void_p, &p.buffer, &buflen) == 0)
    return *p.c_ptr;
  
  std::cerr << "Error: PyObject_AsReadBuffer failed\n";
}




// raise_evo_error is intended to be used when an ERSP call returns a
// result other than RESULT_SUCCESS.  It raises an exception of type
// Evo.EvoError that has its result attribute set to the specified
// result code.

static PyObject *SIFTErrorFactory = NULL;
static PyObject *PyExc_SIFTError = NULL;

static void raise_sift_error(const std::string& msg)
{
  std::cerr << "Raising SIFTError: " << msg << "\n";
  if (!SIFTErrorFactory) {
    PyObject *mod = PyImport_ImportModule("sift");
    if (!mod) std::cerr << "Error: Unable to find module sift\n";
    SIFTErrorFactory = PyObject_GetAttrString(mod, "make_sift_error");
    if (!SIFTErrorFactory)
      std::cerr << "Error: Unable to find function make_sift_error in module sift.\n";
    PyExc_SIFTError = PyObject_GetAttrString(mod, "SIFTError");
    if (!PyExc_SIFTError)
      std::cerr << "Error: Unable to find class SIFTError\n";
  }
  
  PyObject *m = PyString_FromString(msg.c_str());
  PyObject *args = Py_BuildValue("(O)", m);
  PyObject *exc = PyObject_Call(SIFTErrorFactory, args, NULL);
  PyErr_SetObject(PyExc_SIFTError, exc);
  Py_XDECREF(exc);
  Py_XDECREF(args);
  Py_XDECREF(m);
}


extern "C" unsigned remove_ref(SIFT::SObject *obj)
{
  return obj->remove_ref();
}

extern "C" SIFT::Database* sift_database_new()
{
  return new SIFT::Database();
}

extern "C" bool sift_database_contains_label(SIFT::Database *db, char *label)
{
  return db->contains_label(std::string(label));
}

extern "C" unsigned sift_database_image_count(SIFT::Database *db)
{
  return db->image_count();
}

extern "C" unsigned sift_database_feature_count(SIFT::Database *db)
{
  return db->feature_count();
}



extern "C" void sift_database_add_image_file(SIFT::Database *db, char *path, char *label)
{
  bool result;
  Py_BEGIN_ALLOW_THREADS
    result = db->add_image_file(path, label);
  Py_END_ALLOW_THREADS
  if (result != true) {
    raise_sift_error(SIFT::get_last_error());
  }
}

extern "C" bool sift_database_remove_image(SIFT::Database *db, char *label)
{
  return db->remove_image(label);
}



PyObject* match_results_to_python(const SIFT::MatchResultVector& matches)
{
  PyObject *result = PyList_New(0);
  int num_matches = matches.size();
  for (int i = 0; i < num_matches; i++) {
    const SIFT::MatchResult& r(matches[i]);
    PyObject *tuple = Py_BuildValue("(sff)",
				    r.label.c_str(),
				    r.score,
				    r.percentage);
    PyList_Append(result, tuple);
    Py_XDECREF(tuple);
  }

  return result;
}


static PyObject* pysift_database_match_image_file(PyObject *self, PyObject *args)
{
  char *file_path;
  PyObject *pydb;
  SIFT::Database *db;
  if (!PyArg_ParseTuple(args, "O!s", siftdb_t, &pydb, &file_path)) return NULL;
  db = (SIFT::Database*)get_c_ptr(pydb);

  // Load image file.
  IplImage *image = cvLoadImage(file_path, 1);
  if (!image) {
    raise_sift_error(std::string("Unable to load image file ") + 
		     std::string(file_path));
    return NULL;
  }

  // Extract features from image.
  struct feature *features;
  int num_features = sift_features(image, &features);
  cvReleaseImage(&image);

  // Do match.
  int max_nn_checks = 200;
  SIFT::MatchResultVector results(db->match(features, num_features, max_nn_checks));
  PyObject *result = match_results_to_python(results);
  return result;
}

/*
extern "C" SIFT::MatchResultVector* sift_database_match_image_file(SIFT::Database *db, char *path)
{
  int max_nn_checks = 200;
  IplImage *image = cvLoadImage(path, 1);
  struct feature *features;
  int num_features = sift_features(image, &features);
  SIFT::MatchResultVector *results = new SIFT::MatchResultVector(db->match(features, num_features, max_nn_checks));
  return results;
}
*/




// ----------------------------------------
// Python module boilerplate
// ----------------------------------------

static PyMethodDef SIFTMethods[] = {
  {"database_match_image_file", pysift_database_match_image_file, METH_VARARGS, ""},
  //  {"db_match_image", db_match_image, METH_VARARGS, ""},
  //  {"db_fast_load", db_fast_load, METH_VARARGS, ""},
  //  {"kl_get_features", kl_get_features, METH_VARARGS, ""},
  {NULL, NULL, 0, NULL}        /* Sentinel */
};

PyMODINIT_FUNC init_sift(void)
{
  Py_InitModule("_sift", SIFTMethods);
  PyObject* ctypes = PyImport_ImportModule("ctypes");
  siftdb_t =  PyObject_GetAttrString(ctypes, "c_void_p");
  // evoimage_t =  PyObject_GetAttrString(ctypes, "c_void_p");
  // evokl_t = PyObject_GetAttrString(ctypes, "c_void_p");
}
