"""
A vtkTkImageViewerWidget for python, which is based on the
vtkTkImageWindowWidget.

Specify double=1 to get a double-buffered window.

Created by David Gobbi, Nov 1999
"""

import Tkinter
from Tkinter import *
import math, os, sys
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
            if __name__ == "__main__":
                tkWidgetPath = os.path.dirname(os.path.abspath(sys.argv[0]))
            else:
                tkWidgetPath = os.path.abspath(os.path.dirname(__file__))

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

        self.BindTkImageViewer()

    def GetImageViewer(self):
        return self.__ImageViewer

    def Render(self):
        self.__ImageViewer.Render()

    def BindTkImageViewer(self):
        imager = self.__ImageViewer.GetImager()
        
        # stuff for window level text.
        mapper = vtkTextMapper()
        mapper.SetInput("none")
        mapper.SetFontFamilyToTimes()
        mapper.SetFontSize(18)
        mapper.BoldOn()
        mapper.ShadowOn()
        
        self.__LevelMapper = mapper

        actor = vtkActor2D()
        actor.SetMapper(mapper)
        actor.SetLayerNumber(1)
        actor.GetPositionCoordinate().SetValue(4,22)
        actor.GetProperty().SetColor(1,1,0.5)
        actor.SetVisibility(0)
        imager.AddActor2D(actor)

        self.__LevelActor = actor
                
        mapper = vtkTextMapper()
        mapper.SetInput("none")
        mapper.SetFontFamilyToTimes()
        mapper.SetFontSize(18)
        mapper.BoldOn()
        mapper.ShadowOn()
        
        self.__WindowMapper = mapper

        actor = vtkActor2D()
        actor.SetMapper(mapper)
        actor.SetLayerNumber(1)
        actor.GetPositionCoordinate().SetValue(4,4)
        actor.GetProperty().SetColor(1,1,0.5)
        actor.SetVisibility(0)
        imager.AddActor2D(actor)

        self.__WindowActor = actor

        self.__LastX = 0
        self.__LastY = 0
        self.__OldFocus = 0
        self.__InExpose = 0
        
        # bindings
        # window level
        self.bind("<ButtonPress-1>",
                  lambda e,s=self: s.StartWindowLevelInteraction(e.x,e.y))
        self.bind("<B1-Motion>",
                  lambda e,s=self: s.UpdateWindowLevelInteraction(e.x,e.y))
        self.bind("<ButtonRelease-1>",
                  lambda e,s=self: s.EndWindowLevelInteraction()) 
        
        # Get the value
        self.bind("<ButtonPress-3>",
                  lambda e,s=self: s.StartQueryInteraction(e.x,e.y))
        self.bind("<B3-Motion>",
                  lambda e,s=self: s.UpdateQueryInteraction(e.x,e.y))
        self.bind("<ButtonRelease-3>",
                  lambda e,s=self: s.EndQueryInteraction()) 
        
        self.bind("<Expose>",
                  lambda e,s=self: s.ExposeTkImageViewer())
        self.bind("<Enter>",
                  lambda e,s=self: s.EnterTkViewer())
        self.bind("<Leave>",
                  lambda e,s=self: s.LeaveTkViewer())
        self.bind("<KeyPress-e>",
                  lambda e,s=self: s.quit())
        self.bind("<KeyPress-r>",
                  lambda e,s=self: s.ResetTkImageViewer())

    def GetImageViewer(self):
        return self.__ImageViewer

    def Render(self):
        self.__ImageViewer.Render()
        
    def EnterTkViewer(self):
        self.__OldFocus=self.focus_get()
        self.focus()

    def LeaveTkViewer(self):
        if (self.__OldFocus != None):
            self.__OldFocus.focus()

    def ExposeTkImageViewer(self):
        if (self.__InExpose == 0):
            self.__InExpose = 1
            self.update()
            self.__ImageViewer.Render()
            self.__InExpose = 0

    def StartWindowLevelInteraction(self,x,y):
        viewer = self.__ImageViewer
        self.__LastX = x
        self.__LastY = y
        self.__Window = float(viewer.GetColorWindow())
        self.__Level = float(viewer.GetColorLevel())

        # make the window level text visible
        self.__LevelActor.SetVisibility(1)
        self.__WindowActor.SetVisibility(1)

        self.UpdateWindowLevelInteraction(x,y)

    def EndWindowLevelInteraction(self):
        # make the window level text invisible
        self.__LevelActor.SetVisibility(0)
        self.__WindowActor.SetVisibility(0)
        self.Render()

    def UpdateWindowLevelInteraction(self,x,y):
        # compute normalized delta
        dx = 4.0*(x - self.__LastX)/self.winfo_width()*self.__Window
        dy = 4.0*(self.__LastY - y)/self.winfo_height()*self.__Level

        # abs so that direction does not flip
        if (self.__Window < 0.0):
            dx = -dx
        if (self.__Level < 0.0):
            dy = -dy

        # compute new window level
        window = self.__Window + dx
        if (window < 0.0):
            level = self.__Level + dy
        else:
            level = self.__Level - dy

        viewer = self.__ImageViewer
        viewer.SetColorWindow(window)
        viewer.SetColorLevel(level)

        self.__WindowMapper.SetInput("Window: %g" % window)
        self.__LevelMapper.SetInput("Level: %g" % level)
        
        self.Render()


    def ResetTkImageViewer(self):
        # Reset: Set window level to show all values
        viewer = self.__ImageViewer
        input = viewer.GetInput()
        if (input == None):
            return

        # Get the extent in viewer
        z = viewer.GetZSlice()

        input.SetUpdateExtent(-99999,99999,-99999,99999,z,z)
        input.Update()

        (low,high) = input.GetScalarRange()
   
        viewer.SetColorWindow(high - low)
        viewer.SetColorLevel((high + low) * 0.5)

        self.Render()
   
    def StartQueryInteraction(self,x,y):
        # Query PixleValue stuff
        self.__WindowActor.SetVisibility(1)
        self.UpdateQueryInteraction(x,y)

    def EndQueryInteraction(self):
        self.__WindowActor.SetVisibility(0)
        self.Render()

    def UpdateQueryInteraction(self,x,y):
        viewer = self.__ImageViewer
        input = viewer.GetInput()
        z = viewer.GetZSlice()

        # y is flipped upside down
        y = self.winfo_height() - y

        # make sure point is in the whole extent of the image.
        (xMin,xMax,yMin,yMax,zMin,zMax) = input.GetWholeExtent()
        if (x < xMin or x > xMax or y < yMin or \
            y > yMax or z < zMin or z > zMax):
            return

        input.SetUpdateExtent(x,x,y,y,z,z)
        input.Update()
        numComps = input.GetNumberOfScalarComponents()
        text = ""
        for i in xrange(numComps):
            val = input.GetScalarComponentAsFloat(x,y,z,i)
            text = "%s  %.1f" % (text,val)  

        self.__WindowMapper.SetInput("(%d, %d): %s" % (x,y,text))
        
        self.Render()

#support both names
vtkImageViewerWidget = vtkTkImageViewerWidget


# an example of how to use this widget
if __name__ == "__main__":
    canvas = vtkImageCanvasSource2D()
    canvas.SetNumberOfScalarComponents(3)
    canvas.SetScalarType(3)
    canvas.SetExtent(0,511,0,511,0,0)
    canvas.SetDrawColor(100,100,0)
    canvas.FillBox(0,511,0,511)
    canvas.SetDrawColor(200,0,200)
    canvas.FillBox(32,511,100,500)
    canvas.SetDrawColor(100,0,0)
    canvas.FillTube(550,20,30,400,5)
    canvas.SetDrawColor(255,255,255)
    canvas.DrawSegment3D(10,20,0,90,510,0)
    canvas.SetDrawColor(200,50,50)
    canvas.DrawSegment3D(510,90,0,10,20,0)

    # Check segment clipping
    canvas.SetDrawColor(0,200,0)
    canvas.DrawSegment(-10,30,30,-10)
    canvas.DrawSegment(-10,481,30,521)
    canvas.DrawSegment(481,-10,521,30)
    canvas.DrawSegment(481,521,521,481)

    # Check Filling a triangle
    canvas.SetDrawColor(20,200,200)
    canvas.FillTriangle(-100,100,190,150,40,300)

    # Check drawing a circle
    canvas.SetDrawColor(250,250,10)
    canvas.DrawCircle(350,350,200.0)

    # Check drawing a point
    canvas.SetDrawColor(250,250,250)
    canvas.DrawPoint(350,350)
    canvas.DrawPoint(350,550)

    # Test filling functionality
    canvas.SetDrawColor(55,0,0)
    canvas.DrawCircle(450,350,80.0)
    canvas.SetDrawColor(100,255,100)
    canvas.FillPixel(450,350)

    viewer = vtkImageViewer()
    viewer.SetInput(canvas.GetOutput())
    viewer.SetColorWindow(256)
    viewer.SetColorLevel(127.5)
    viewer.GetImageWindow().DoubleBufferOn()

    # Create the GUI: two renderer widgets and a quit button
    #
    frame = Frame()

    ren = vtkTkImageViewerWidget(frame,width=512,height=512,iv=viewer)

    button = Button(frame,text="Quit",command=frame.quit)

    ren.pack(side='top',padx=3,pady=3,fill='both',expand='t')
    frame.pack(fill='both',expand='t')
    button.pack(fill='x')

    frame.mainloop()

