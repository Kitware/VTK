"""
A simple VTK input file for Python, which includes
a vtkTkRenderWidget for Tkinter.  The widget can be
accessed under the names 'vtkTkRenderWidget' or 'vtkRenderWidget'
 
Created by David Gobbi, April 1999

May ??, 1999  - Modifications peformed by Heather Drury,
                to rewrite _pan to match method in TkInteractor.tcl
May 11, 1999  - Major rewrite by David Gobbi to make the
                interactor bindings identical to the TkInteractor.tcl 
                bindings.
July 14, 1999 - Added modification by Ken Martin for VTK 2.4, to
                use vtk widgets instead of Togl.
Aug 29, 1999  - Renamed file to vtkRenderWidget.py
Nov 14, 1999  - Added support for keyword 'rw'
"""

import Tkinter
from Tkinter import *
import math, os
from vtkpython import *

class vtkTkRenderWidget(Tkinter.Widget):
    """
    A vtkTkRenderWidget for Python.
    Use GetRenderWindow() to get the vtkRenderWindow.
    Create with the keyword stereo=1 in order to
    generate a stereo-capable window.
    """
    def __init__(self, master, cnf={}, **kw):
        try: # check for VTK_TK_WIDGET_PATH environment variable
	    tkWidgetPath = os.environ['VTK_TK_WIDGET_PATH']
        except KeyError:
            tkWidgetPath = "."

        try: # try specified path or current directory
            master.tk.call('load',os.path.join(tkWidgetPath, \
                                               'vtkTkRenderWidget'))
        except: # try tcl/tk load path
            master.tk.call('load','vtkTkRenderWidget')

        try: # check to see if a render window was specified
            self.__RenderWindow = kw['rw']
        except KeyError:
            self.__RenderWindow = vtkRenderWindow()

        try:  # was a stereo rendering context requested?
            if kw['stereo']:
	       self.__RenderWindow.StereoCapableWindowOn()
               del kw['stereo']
	except KeyError:
            pass
 
        kw['rw'] = self.__RenderWindow.GetAddressAsString("vtkRenderWindow")
        Tkinter.Widget.__init__(self, master, 'vtkTkRenderWidget', cnf, kw)

        # initialize some global variables        
        self.__InExpose = 0
        self.__CurrentRenderer = None
        self.__PickedAssembly = None
        self.__ActorPicker = vtkCellPicker()
        self.__PickedProperty = vtkProperty()
        self.__PickedProperty.SetColor(1,0,0)
        self.__PrePickedProperty = None
  
        self.BindTkRenderWidget()

    def BindTkRenderWidget(self):
        self.bind("<Any-ButtonPress>",
                  lambda e,s=self: s.StartMotion(e.x,e.y))
        self.bind("<Any-ButtonRelease>",
                  lambda e,s=self: s.EndMotion(e.x,e.y))
        self.bind("<B1-Motion>",
                  lambda e,s=self: s.Rotate(e.x,e.y))
        self.bind("<B2-Motion>",
                  lambda e,s=self: s.Pan(e.x,e.y))
        self.bind("<B3-Motion>",
                  lambda e,s=self: s.Zoom(e.x,e.y))
        self.bind("<Shift-B1-Motion>",
                  lambda e,s=self: s.Pan(e.x,e.y))
        self.bind("<KeyPress-r>",
                  lambda e,s=self: s.Reset(e.x,e.y))
        self.bind("<KeyPress-u>",
                  lambda e,s=self: s.deiconify())
        self.bind("<KeyPress-w>",
                  lambda e,s=self: s.Wireframe())
        self.bind("<KeyPress-s>",
                  lambda e,s=self: s.Surface())
        self.bind("<KeyPress-p>",
                  lambda e,s=self: s.PickActor(e.x,e.y))
        self.bind("<Enter>",
                  lambda e,s=self: s.Enter(e.x,e.y))
        self.bind("<Leave>",
                  lambda e,s=self: s.Leave(e.x,e.y))
        self.bind("<Expose>",
                  lambda e,s=self: s.Expose())

    def GetRenderWindow(self):
        return self.__RenderWindow

    def Expose(self):
        if (self.__InExpose == 0):
            self.__InExpose = 1
            self.update()
            self.__RenderWindow.Render()
            self.__InExpose = 0

    def Render(self):
        self.__CurrentLight.SetPosition(self.__CurrentCamera.GetPosition())
        self.__CurrentLight.SetFocalPoint(self.__CurrentCamera.GetFocalPoint())
        self.__RenderWindow.Render()

    def UpdateRenderer(self,x,y):
        windowX = self.winfo_width()
        windowY = self.winfo_height()

        renderers = self.__RenderWindow.GetRenderers()
        numRenderers = renderers.GetNumberOfItems()

        renderers.InitTraversal()
        for i in range(0,numRenderers):
            renderer = renderers.GetNextItem()
            vx = float(x)/windowX
            vy = (windowY-float(x))/windowY
            (vpxmin,vpymin,vpxmax,vpymax) = renderer.GetViewport()
            if (vx >= vpxmin and vy <= vpxmax and
                vy >= vpymin and vy <= vpymax):
                self.__RendererFound = 1
                self.__CurrentRenderer = renderer
                self.__WindowCenterX = float(windowX)*(vpxmax-vpxmin)/2.0\
                                       +vpxmin
                self.__WindowCenterY = float(windowY)*(vpymax-vpymin)/2.0\
                                       +vpymin
                self.__CurrentCamera = self.__CurrentRenderer.GetActiveCamera()
                lights = self.__CurrentRenderer.GetLights()
                lights.InitTraversal()
                self.__CurrentLight = lights.GetNextItem()

                break

        self.__LastX = x
        self.__LastY = y
                
    def Enter(self,x,y):
        self.__OldFocus=self.focus_get()
        self.focus()
        self.UpdateRenderer(x,y)

    def Leave(self,x,y):
        if (self.__OldFocus != None):
            self.__OldFocus.focus()

    def StartMotion(self,x,y):
        self.UpdateRenderer(x,y)
        if self.__RendererFound:
            self.__RenderWindow.SetDesiredUpdateRate(1.0)

    def EndMotion(self,x,y):
        if self.__RendererFound:
            self.__RenderWindow.SetDesiredUpdateRate(0.1)
            self.Render()

    def Rotate(self,x,y):
        if self.__RendererFound:
            
            self.__CurrentCamera.Azimuth(self.__LastX - x)
            self.__CurrentCamera.Elevation(y - self.__LastY)
            self.__CurrentCamera.OrthogonalizeViewUp()
            
            self.__LastX = x
            self.__LastY = y
            
            self.Render()

    def Pan(self,x,y):
        if self.__RendererFound:
            
            renderer = self.__CurrentRenderer
            camera = self.__CurrentCamera

            (fPoint0,fPoint1,fPoint2) = camera.GetFocalPoint()
            (pPoint0,pPoint1,pPoint2) = camera.GetPosition()

            # Specify a point location in world coordinates
            renderer.SetWorldPoint(fPoint0,fPoint1,fPoint2,1.0)
            renderer.WorldToDisplay()
            # Convert world point coordinates to display coordinates
            dPoint = renderer.GetDisplayPoint()
            focalDepth = dPoint[2]

            aPoint0 = self.__WindowCenterX + (x - self.__LastX)
            aPoint1 = self.__WindowCenterY - (y - self.__LastY)

            renderer.SetDisplayPoint(aPoint0,aPoint1,focalDepth)
            renderer.DisplayToWorld()

            (rPoint0,rPoint1,rPoint2,rPoint3) = renderer.GetWorldPoint()
            if (rPoint3 != 0.0):
                rPoint0 = rPoint0/rPoint3
                rPoint1 = rPoint1/rPoint3
                rPoint2 = rPoint2/rPoint3

            camera.SetFocalPoint((fPoint0 - rPoint0)/2.0 + fPoint0, 
                                 (fPoint1 - rPoint1)/2.0 + fPoint1,
                                 (fPoint2 - rPoint2)/2.0 + fPoint2) 

            camera.SetPosition((fPoint0 - rPoint0)/2.0 + pPoint0, 
                               (fPoint1 - rPoint1)/2.0 + pPoint1,
                               (fPoint2 - rPoint2)/2.0 + pPoint2)

            self.__LastX = x
            self.__LastY = y

            self.Render()

    def Zoom(self,x,y):
        if self.__RendererFound:

            renderer = self.__CurrentRenderer
            camera = self.__CurrentCamera

            zoomFactor = math.pow(1.02,(0.5*(y - self.__LastY)))

            if camera.GetParallelProjection():
                parallelScale = camera.GetParallelScale() * zoomFactor
                camera.SetParallelScale(parallelScale)
            else:
                camera.Dolly(zoomFactor)
                renderer.ResetCameraClippingRange()

            self.__LastX = x
            self.__LastY = y

            self.Render()

    def Reset(self,x,y):
        windowX = self.winfo_width()
        windowY = self.winfo_height()

        renderers = self.__RenderWindow.GetRenderers()
        numRenderers = renderers.GetNumberOfItems()

        renderers.InitTraversal()
        for i in range(0,numRenderers):
            renderer = renderers.GetNextItem()
            vx = float(x)/windowX
            vy = (windowY-float(x))/windowY
            (vpxmin,vpymin,vpxmax,vpymax) = renderer.GetViewport()
            if (vx >= vpxmin and vy <= vpxmax and
                vy >= vpymin and vy <= vpymax):
                self.__RendererFound = 1
                self.__CurrentRenderer = renderer
                windowCenterX = float(windowX)*(vpxmax-vpxmin)/2.0+vpxmin
                windowCenterY = float(windowY)*(vpymax-vpymin)/2.0+vpymin
                break

        if self.__RendererFound:
            self.__CurrentRenderer.ResetCamera()

        self.Render()

    def Wireframe(self):
        actors = self.__CurrentRenderer.GetActors()
        numActors = actors.GetNumberOfItems()
        actors.InitTraversal()
        for i in range(0,numActors):
            actor = actors.GetNextItem()
            actor.GetProperty().SetRepresentationToWireframe()

        self.Render()
        
    def Surface(self):
        actors = self.__CurrentRenderer.GetActors()
        numActors = actors.GetNumberOfItems()
        actors.InitTraversal()
        for i in range(0,numActors):
            actor = actors.GetNextItem()
            actor.GetProperty().SetRepresentationToSurface()

        self.Render()

    def PickActor(self,x,y):
        if self.__RendererFound:

            renderer = self.__CurrentRenderer
            actorPicker = self.__ActorPicker
            
            windowY = self.winfo_height()
            actorPicker.Pick(x,(windowY - y - 1),0.0,renderer)
            assembly = actorPicker.GetAssembly()

            if (self.__PickedAssembly != None and
                self.__PrePickedProperty != None):
                self.__PickedAssembly.SetProperty(self.__PrePickedProperty)
                # release hold of the property
                self.__PrePickedProperty.UnRegister(self.__PrePickedProperty)
                self.__PrePickedProperty = None

            if (assembly != None):
                self.__PickedAssembly = assembly
                self.__PrePickedProperty = self.__PickedAssembly.GetProperty()
                # hold onto the property
                self.__PrePickedProperty.Register(self.__PrePickedProperty)
                self.__PickedAssembly.SetProperty(self.__PickedProperty)

            self.Render()
    
#support both names
vtkRenderWidget = vtkTkRenderWidget



