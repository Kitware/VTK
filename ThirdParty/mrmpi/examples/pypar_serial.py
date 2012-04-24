# dummy Pypar routines
# can use if running in serial, and Pypar isn't installed
# Pypar (http://datamining.anu.edu.au/~ole/pypar) is a Python wrapper on MPI

import time as clock

def finalize():
  pass

def size():
  return 1

def rank():
  return 0

def barrier():
  pass

def time():
  return clock.clock()
