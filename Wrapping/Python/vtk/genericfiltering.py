""" This module loads all the classes from the VTK GenericFiltering
library into its namespace.  This is an optional module."""

import os  

if os.name == 'posix':
    from libvtkGenericFilteringPython import *
else:
    from vtkGenericFilteringPython import *
