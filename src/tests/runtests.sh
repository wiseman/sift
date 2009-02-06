#!/bin/bash

export LD_LIBRARY_PATH=../.libs:"${LD_LIBRARY_PATH}"
export PYTHONPATH=..:../.libs:"${PYTHONPATH}"

./test_sift.py $@
