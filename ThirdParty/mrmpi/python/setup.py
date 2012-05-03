#!/usr/local/bin/python

"""
setup.py file for MapReduce MPI files which call system MPI library
"""

from distutils.core import setup, Extension

import os, glob
path = os.path.dirname(os.getcwd())
libfiles = glob.glob("%s/src/*.cpp" % path)

mrmpi_library = Extension("_mrmpi",
                          sources = libfiles,
                          define_macros = [("MPICH_IGNORE_CXX_SEEK",1)],
                          include_dirs = ["../src"],
                          library_dirs = ["/usr/local/lib"],
                          # works with MPICH on Linux
                          libraries = ["mpich","rt"],
                          # works on a Mac with default MPI
                          # libraries = ["mpi"],
                          )

setup(name = "mrmpi",
      version = "7Apr09",
      author = "Steve Plimpton",
      author_email = "sjplimp@sandia.gov",
      url = "http://www.cs.sandia.gov/mapreduce.html",
      description = """MapReduce MPI library""",
      py_modules = ["mrmpi"],
      ext_modules = [mrmpi_library]
      )
