""" This module loads all the classes from the VTK/Qt libraries into
its namespace.  This is an optional module."""

import os

from QVTKPython import *

if os.name == 'posix':
    from libvtkQtPython import *
else:
    from vtkQtPython import *
