""" This module loads all the classes from the VTK Imaging library into
its namespace.  This is a required module."""

import os

if os.name == 'posix':
    from libvtkImagingPython import *
else:
    from vtkImagingPython import *
