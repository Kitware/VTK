""" This module loads all the classes from the VTK Views library into
its namespace.  This is an optional module."""

import os

if os.name == 'posix':
    from libvtkViewsPython import *
else:
    from vtkViewsPython import *
