""" This module loads all the classes from the VTK Parallel library into
its namespace.  This is an optional module."""

import os

if os.name == 'posix':
    from libvtkParallelPython import *
else:
    from vtkParallelPython import *
