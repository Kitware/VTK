""" This module loads all the classes from the VTK Text Analysis library into
its namespace.  This is an optional module."""

import os

if os.name == 'posix':
    from libvtkTextAnalysisPython import *
else:
    from vtkTextAnalysisPython import *
