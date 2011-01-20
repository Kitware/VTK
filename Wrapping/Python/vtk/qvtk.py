""" This module loads all the classes from the VTK/Qt libraries into
its namespace.  This is an optional module."""

# library for VTK classes with Qt support (load first)
from vtkQtPython import *

# library for Qt classes with VTK support
from QVTKPython import *
