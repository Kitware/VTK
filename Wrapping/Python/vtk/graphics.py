""" This module loads all the classes from the VTK Graphics library into its
namespace.  This is a required module."""

import os

if os.name == 'posix':
    from libvtkGraphicsPython import *
else:
    from vtkGraphicsPython import *
