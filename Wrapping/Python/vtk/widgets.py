""" This module loads all the classes from the VTK Widgets library
into its namespace.  This is an optional module."""

import os  

if os.name == 'posix':
    from libvtkWidgetsPython import *
else:
    from vtkWidgetsPython import *
