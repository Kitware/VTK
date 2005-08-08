"""
A subclass of Tkinter.PhotoImage that connects a
vtkImageData to a photo widget.

Created by Daniel Blezek, August 2002
"""

import Tkinter

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
    def PutImageSlice ( self, image, z, orientation='transverse', window=256, level=128 ):
        t = image.__this__
        s = 'vtkImageDataToTkPhoto %s %s %d %s %d %d' % ( t[:-2], self.name, z, orientation, window, level )
        self.tk.eval ( s )
