"""
A subclass of Tkinter.PhotoImage that connects a
vtkImageData to a photo widget.

Created by Daniel Blezek, August 2002
"""

import Tkinter
from Tkinter import *
from vtkpython import *

from vtkLoadPythonTkWidgets import vtkLoadPythonTkWidgets

class vtkTkPhotoImage ( Tkinter.PhotoImage ):
    """
    A subclass of PhotoImage with helper functions
    for displaying vtkImageData
    """
    def __init__ ( self, **kw ):
        # Caller the superclass
        Tkinter.PhotoImage.__init__ ( self, kw )
        vtkLoadPythonTkWidgets ( self.tk )
    def PutImageSlice ( self, image, z ):
        t = cast.GetOutput().__this__
        photo.tk.eval ( 'vtkImageDataToTkPhoto ' + t[:-2] + ' ' + self.name + ' ' + z );

