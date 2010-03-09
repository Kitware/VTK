""" This module loads all the classes from the VTK Charts library into
its namespace.  This is an optional module."""

import os

if os.name == 'posix':
    from libvtkChartsPython import *
else:
    from vtkChartsPython import *
