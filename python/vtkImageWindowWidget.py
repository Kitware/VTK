"""
A vtkTkImageWindowWidget for python.
Specify double=1 to get a double-buffered window.

Created by David Gobbi, Nov 1999
"""

import Tkinter
from Tkinter import *
import math, os
from vtkpython import *

class vtkTkImageWindowWidget(Tkinter.Widget):
    """
    A vtkTkImageWindowWidget for Python.
    Use GetImageWindow() to get the vtkImageWindow.
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

        try: # use specified vtkImageWindow
            self.__ImageWindow = kw['iw']
            kw['iw'] = kw['iw'].GetAddressAsString("vtkImageWindow")
        except KeyError: # or create one if none specified
            self.__ImageWindow = vtkImageWindow()
            kw['iw'] = self.__ImageWindow.GetAddressAsString("vtkImageWindow")
            
        try:  # was a double-buffer rendering context requested?
            if kw['double']:
	       self.__ImageWindow.DoubleBufferOn()
               del kw['double']
	except KeyError:
            pass

        Tkinter.Widget.__init__(self, master, 'vtkTkImageWindowWidget',
                                cnf, kw)

        self.__InExpose = 0
        self.bind("<Expose>",lambda e,s=self: s.Expose())

    def GetImageWindow(self):
        return self.__ImageWindow

    def Render(self):
        self.__ImageWindow.Render()

    def Expose(self):
        if (self.__InExpose == 0):
            self.__InExpose = 1
            self.update()
            self.__ImageWindow.Render()
            self.__InExpose = 0

#support both names
vtkImageWindowWidget = vtkTkImageWindowWidget



