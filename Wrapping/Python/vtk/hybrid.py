""" This module loads all the classes from the VTK Hybrid library into
its namespace.  This is an optional module."""

import os

if os.name == 'posix':
    from libvtkHybridPython import *
else:
    from vtkHybridPython import *
