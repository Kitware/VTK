""" This module loads all the classes from the VTK Patented library into
its namespace.  This is an optional module."""

import os

if os.name == 'posix':
    from libvtkPatentedPython import *
else:
    from vtkPatentedPython import *
