"""
A vtkTkImageWindowWidget for python.
Specify double=1 to get a double-buffered window.

Created by David Gobbi, Nov 1999
"""

import Tkinter
from Tkinter import *
import math, os, sys
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
            if __name__ == "__main__":
                tkWidgetPath = os.path.dirname(os.path.abspath(sys.argv[0]))
            else:
                tkWidgetPath = os.path.abspath(os.path.dirname(__file__))

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

    imgWin = vtkImageWindow()
    imgWin.AddImager(imager1)

    top = Tk()
    top_f1 = Frame(top)

    top_f1_r1 = vtkTkImageWindowWidget(top_f1,width=256,height=256,iw=imgWin)

    top_btn = Button(top,text="Quit",command=top.tk.quit)

    top_f1_r1.pack(side='left',padx=3,pady=3,fill='both',expand='t')
    top_f1.pack(fill='both',expand='t')
    top_btn.pack(fill='x')

    imager1.SetBackground(0.1,0.0,0.6)

    top.mainloop()


