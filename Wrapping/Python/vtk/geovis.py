""" This module loads all the classes from the VTK Geovis library into
its namespace.  This is an optional module."""

import os

if os.name == 'posix':
    from libvtkGeovisPython import *
else:
    from vtkGeovisPython import *
