"""
VTK.py 
An VTK module for python that includes:

Wrappers for all the VTK classes that are wrappable

A Tkinter vtkRenderWidget (works like the tcl vtkTkRenderWidget)
The vtkImageViewerWidget and vtkImageWindowWidget are coming soon.

Classes to assist in moving data between python and VTK.
"""

from vtkpython import *

print "VTK Version", vtkVersion().GetVTKVersion()

from vtkConstants import *

from vtkRenderWidget import vtkRenderWidget,vtkTkRenderWidget

#from vtkImageWindowWidget import vtkImageWindowWidget,vtkTkImageWindowWidget
#from vtkImageViewerWidget import vtkImageViewerWidget,vtkTkImageViewerWidget

from vtkImageImportFromArray import *
from vtkImageExportToArray import *

