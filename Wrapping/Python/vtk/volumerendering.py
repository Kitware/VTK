""" This module loads all the classes from the VTK VolumeRendering library into
its namespace.  This is an optional module."""

import os

if os.name == 'posix':
    from libvtkVolumeRenderingPython import *
else:
    from vtkVolumeRenderingPython import *
