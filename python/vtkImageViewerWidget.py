"""
A vtkTkImageViewerWidget for python, which is based on the
vtkTkImageWindowWidget.

Specify double=1 to get a double-buffered window.

Created by David Gobbi, Nov 1999

WARNING -- this file is not finished yet, the standard vtkTkImageViewer
           bindings are not supported
"""

import Tkinter
from Tkinter import *
import math, os
from vtkpython import *

class vtkTkImageViewerWidget(Tkinter.Widget):
    """
    A vtkTkImageViewerWidget for Python.
    Use GetImageViewer() to get the vtkImageViewer.
    Create with the keyword double=1 in order to
    generate a double-buffered viewer.
    """
    def __init__(self, master, cnf={}, **kw):
        try: # check for VTK_TK_WIDGET_PATH environment variable
	    tkWidgetPath = os.environ['VTK_TK_WIDGET_PATH']
        except KeyError:
            tkWidgetPath = "."

        try: # try specified path or current directory
            master.tk.call('load',os.path.join(tkWidgetPath, \
                                               'vtkTkImageWindowWidget'))
        except: # try tcl/tk load path
            master.tk.call('load','vtkTkImageWindowWidget')


        try: # use specified vtkImageViewer
            self.__ImageViewer = kw['iv']
            del kw['iv']
        except KeyError: # or create one if none specified
            self.__ImageViewer = vtkImageViewer()

        kw['iw'] = self.__ImageViewer.GetImageWindow().\
                                    GetAddressAsString("vtkImageWindow")
            
        try:  # was a double-buffer rendering context requested?
            if kw['double']:
	       self.__ImageViewer.GetImageWindow().DoubleBufferOn()
               del kw['double']
	except KeyError:
            pass

        Tkinter.Widget.__init__(self, master, 'vtkTkImageWindowWidget',
                                cnf, kw)

        self.__InExpose = 0
        self.bind("<Expose>",lambda e,s=self: s.Expose())

    def GetImageViewer(self):
        return self.__ImageViewer

    def Render(self):
        self.__ImageViewer.Render()

    def Expose(self):
        if (self.__InExpose == 0):
            self.__InExpose = 1
            self.update()
            self.__ImageViewer.Render()
            self.__InExpose = 0

#support both names
vtkImageViewerWidget = vtkTkImageViewerWidget



