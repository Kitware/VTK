"""
A vtkTkImageWindowWidget for python.
Specify double=1 to get a double-buffered window.

Created by David Gobbi, Nov 1999
Update for VTK 4, Aug 2001
"""

import Tkinter
from Tkinter import *
import math, os, sys
from vtkpython import *

from vtkLoadPythonTkWidgets import vtkLoadPythonTkWidgets

class vtkTkImageWindowWidget(Tkinter.Widget):
    """
    A vtkTkImageWindowWidget for Python.
    Use GetImageWindow() to get the vtkImageWindow.
    """
    def __init__(self, master, cnf={}, **kw):
        # load the necessary extensions into tk
        vtkLoadPythonTkWidgets(master.tk)        

        try: # use specified vtkImageWindow
            imageWindow = kw['iw']
        except KeyError: # or create one if none specified
            imageWindow = vtkImageWindow()

        doubleBuffer = 0
        try:
            if kw['double']:
                doubleBuffer = 1
            del kw['double']
        except:
            pass
            
        kw['iw'] = imageWindow.GetAddressAsString("vtkImageWindow")
        Tkinter.Widget.__init__(self, master, 'vtkTkImageWindowWidget',
                                cnf, kw)
        if doubleBuffer:
            imageWindow.DoubleBufferOn()        

        self.__InExpose = 0
        self.bind("<Expose>",lambda e,s=self: s.Expose())

    def __getattr__(self,attr):
        # because the tk part of vtkTkImageWindowWidget must have
        # the only remaining reference to the ImageWindow when
        # it is destroyed, we can't actually store the ImageWindow
        # as an attribute but instead have to get it from the tk-side
        if attr == '_ImageWindow':
            addr = self.tk.call(self._w, 'GetImageWindow')[5:]
            return vtkImageWindow('_%s_vtkImageWindow_p' % addr)
        raise AttributeError, self.__class__.__name__ + \
              " has no attribute named " + attr

    def GetImageWindow(self):
        return self._ImageWindow

    def Render(self):
        self._ImageWindow.Render()

    def Expose(self):
        if (self.__InExpose == 0):
            self.__InExpose = 1
            self.update()
            self._ImageWindow.Render()
            self.__InExpose = 0

#-----------------------------------------------------------------------------
# a short how-to-use example
if __name__ == "__main__":
    vtext = vtkVectorText()
    vtext.SetText("Imagine!")

    trans = vtkTransform()
    trans.Scale(25,25,25)

    tpd = vtkTransformPolyDataFilter()
    tpd.SetTransform(trans)
    tpd.SetInput(vtext.GetOutput())

    textMapper = vtkPolyDataMapper2D()
    textMapper.SetInput(tpd.GetOutput())

    coord = vtkCoordinate()
    coord.SetCoordinateSystemToNormalizedViewport()
    coord.SetValue(0.5,0.5)

    textActor = vtkActor2D()
    textActor.SetMapper(textMapper)
    textActor.GetProperty().SetColor(0.7,1.0,1.0)
    textActor.GetPositionCoordinate().SetReferenceCoordinate(coord)
    textActor.GetPositionCoordinate().SetCoordinateSystemToViewport()
    textActor.GetPositionCoordinate().SetValue(-80,-20)

    imager1 = vtkImager()
    imager1.AddActor2D(textActor)

    top = Tk()
    top_f1 = Frame(top)

    widget = vtkTkImageWindowWidget(top_f1,width=256,height=256)
    widget.GetImageWindow().AddImager(imager1)

    top_btn = Button(top,text="Quit",command=top.tk.quit)

    widget.pack(side='left',padx=3,pady=3,fill='both',expand='t')
    top_f1.pack(fill='both',expand='t')
    top_btn.pack(fill='x')

    imager1.SetBackground(0.1,0.0,0.6)

    top.mainloop()


