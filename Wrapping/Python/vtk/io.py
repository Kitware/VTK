""" This module loads all the classes from the VTK IO library into its
namespace.  This is a required module."""

import os  

if os.name == 'posix':
    from libvtkIOPython import *
else:
    from vtkIOPython import *
