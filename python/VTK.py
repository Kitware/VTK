"""
VTK.py 
An VTK module for python that includes:

Wrappers for all the VTK classes that are wrappable

The following are included:

A Tkinter vtkRenderWidget (works like the tcl vtkTkRenderWidget)
Ditto for the vtkImageWindowWidget, vtkImageViewerWidget.

Classes to assist in moving data between python and VTK.
"""

from vtkpython import *
from Tkinter import *

from vtkConstants import *

from vtkRenderWidget import vtkRenderWidget,vtkTkRenderWidget
from vtkImageWindowWidget import vtkImageWindowWidget,vtkTkImageWindowWidget
from vtkImageViewerWidget import vtkImageViewerWidget,vtkTkImageViewerWidget

try:
    from vtkImageImportFromArray import *
    from vtkImageExportToArray import *
except:
    pass
