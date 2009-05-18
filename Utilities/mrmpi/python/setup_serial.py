#!/usr/local/bin/python

"""
setup_serial.py file for MapReduce MPI files with dummy serial MPI library
"""

from distutils.core import setup, Extension

import os, glob
path = os.path.dirname(os.getcwd())
libfiles = glob.glob("%s/src/*.cpp" % path) + \
           glob.glob("%s/mpistubs/*.cpp" % path)

mrmpi_library = Extension("_mrmpi_serial",
                          sources = libfiles,
                          define_macros = [("MPICH_IGNORE_CXX_SEEK",1)],
                          include_dirs = ["../src", "../mpistubs"],
                          )

setup(name = "mrmpi_serial",
      version = "7Apr09",
      author = "Steve Plimpton",
      author_email = "sjplimp@sandia.gov",
      url = "http://www.cs.sandia.gov/mapreduce.html",
      description = """MapReduce MPI library""",
      py_modules = ["mrmpi"],
      ext_modules = [mrmpi_library]
      )
