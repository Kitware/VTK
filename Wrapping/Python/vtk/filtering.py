""" This module loads all the classes from the VTK Filtering library into
its namespace.  This is a required module."""

import os

if os.name == 'posix':
    from libvtkFilteringPython import *
else:
    from vtkFilteringPython import *
