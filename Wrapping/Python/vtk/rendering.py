""" This module loads all the classes from the VTK Rendering library into
its namespace.  This is an optional module."""

import os

if os.name == 'posix':
    from libvtkRenderingPython import *
else:
    from vtkRenderingPython import *
