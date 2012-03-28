"""
A simple VTK input file for PyQt, the qt bindings for python.
See http://www.trolltech.com for qt documentation, and
http://www.river-bank.demon.co.uk or http://www.thekompany.com
for the qt python bindings.

*******************************************************

   NOTE: The widget provided by this module is not free of bugs and it
   is recommended that you consider using the
   QVTKRenderWindowInteractor widget that is also in this directory
   instead of this one.

*******************************************************


Created by David Gobbi, December 2001
Based on vtkTkRenderWindget.py

Changes by Gerard Vermeulen Feb. 2003
 Win32 support
"""

"""
This class should work with both the UNIX version of Qt and also on
Win32.

Depending on the OpenGL graphics drivers, it may not
be possible to have more than one QVTKRenderWidget
per application.

In short, this class is experimental.  A proper implementation
will probably require a QVTKRenderWidget that is written in
C++ and then wrapped to be made available through python,
similar to the vtkTkRenderWidget.
"""

# Problems on Win32:
# 1. The widget is not cleaned up properly and crashes the
#    application.

import vtk
import math, os, sys
from qt import *

class QVTKRenderWidget(QWidget):
    """
    A QVTKRenderWidget for Python and Qt.
    Use GetRenderWindow() to get the vtkRenderWindow.
    Create with the keyword stereo=1 in order to
    generate a stereo-capable window.
    """
    def __init__(self, parent=None, name=None, *args, **kw):

        # miscellaneous protected variables
        self._CurrentRenderer = None
        self._CurrentCamera = None
        self._CurrentZoom = 1.0
        self._CurrentLight = None

        self._ViewportCenterX = 0
        self._ViewportCenterY = 0

        self._Picker = vtk.vtkCellPicker()
        self._PickedActor = None
        self._PickedProperty = vtk.vtkProperty()
        self._PickedProperty.SetColor(1,0,0)
        self._PrePickedProperty = None

        # these record the previous mouse position
        self._LastX = 0
        self._LastY = 0

        # the current interaction mode (Rotate, Pan, Zoom, etc)
        self._Mode = None
        self._ActiveButton = 0

        # used by the LOD actors
        self._DesiredUpdateRate = 15
        self._StillUpdateRate = 0.0001

        # private attributes
        self.__oldFocus = None
        self.__saveX = 0
        self.__saveY = 0
        self.__saveState = 0
        self.__connected = 0  # is QT->VTK connection done?

        # do special handling of some keywords:
        # stereo, rw

        stereo = 0

        if kw.has_key('stereo'):
            if kw['stereo']:
                stereo = 1
            del kw['stereo']

        rw = None

        if kw.has_key('rw'):
            rw = kw['rw']
            del kw['rw']

        # create qt-level widget
        apply(QWidget.__init__, (self,parent,name) + args, kw)

        if rw: # user-supplied render window
            self._RenderWindow = rw
        else:
            self._RenderWindow = vtk.vtkRenderWindow()

        if stereo: # stereo mode
            self._RenderWindow.StereoCapableWindowOn()
            self._RenderWindow.SetStereoTypeToCrystalEyes()

        # do all the necessary qt setup
        self.setBackgroundMode(2) # NoBackground
        self.setMouseTracking(1) # get all mouse events
        self.setFocusPolicy(2) # ClickFocus
        if parent == None:
            self.show()

        if self.isVisible():
            if self.__connected == 0:
                size = self.size()
                self._RenderWindow.SetSize(size.width(),size.height())
                self._RenderWindow.SetWindowInfo(str(int(self.winId())))
                self.__connected = 1


    def show(self):
        QWidget.show(self)
        self.repaint() # needed for initial contents display on Win32

    def paintEvent(self,ev):
        if self.isVisible():
            if self.__connected == 0:
                size = self.size()
                self._RenderWindow.SetSize(size.width(),size.height())
                self._RenderWindow.SetWindowInfo(str(int(self.winId())))
                self.__connected = 1
        if self.__connected:
            self.Render()

    def resizeEvent(self,ev):
        self.repaint()

    def enterEvent(self,ev):
        if not self.hasFocus():
            self.__oldFocus = self.focusWidget()
            self.setFocus()

    def leaveEvent(self,ev):
        if (self.__saveState & 0x7) == 0 and self.__oldFocus:
            self.__oldFocus.setFocus()
            self.__oldFocus = None

    def mousePressEvent(self,ev):
        if self._Mode != None:
            return

        if (ev.button() == 2 or
            ev.button() == 1 and ev.state() & 16):
            self._Mode = "Zoom"
            self._ActiveButton = ev.button()
        elif (ev.button() == 4 or
              ev.button() == 1 and ev.state() & 8):
            self._Mode = "Pan"
            self._ActiveButton = ev.button()
        elif (ev.button() == 1):
            self._Mode = "Rotate"
            self._ActiveButton = ev.button()

        if self._Mode != None:
            self._RenderWindow.SetDesiredUpdateRate(self._DesiredUpdateRate)

        self.UpdateRenderer(ev.x(),ev.y())

    def mouseReleaseEvent(self,ev):
        if self._Mode == None:
            return

        self._RenderWindow.SetDesiredUpdateRate(self._StillUpdateRate)

        if self._CurrentRenderer:
            self.Render()
        if ev.button() == self._ActiveButton:
            self._Mode = None
            self._ActiveButton = 0

    def mouseMoveEvent(self,ev):
        self.__saveState = ev.state()
        self.__saveX = ev.x()
        self.__saveY = ev.y()
        if self._Mode == "Pan":
            self.Pan(ev.x(),ev.y())
        elif self._Mode == "Rotate":
            self.Rotate(ev.x(),ev.y())
        elif self._Mode == "Zoom":
            self.Zoom(ev.x(),ev.y())

    def keyPressEvent(self,ev):
        if ev.key() == ord('R'):
            self.Reset(self.__saveX,self.__saveY)
        if ev.key() == ord('W'):
            self.Wireframe()
        if ev.key() == ord('S'):
            self.Surface()
        if ev.key() == ord('P'):
            self.PickActor(self.__saveX,self.__saveY)

    def contextMenuEvent(self,ev):
        ev.accept();

    def SetDesiredUpdateRate(self, rate):
        """Mirrors the method with the same name in
        vtkRenderWindowInteractor."""
        self._DesiredUpdateRate = rate

    def GetDesiredUpdateRate(self):
        """Mirrors the method with the same name in
        vtkRenderWindowInteractor."""
        return self._DesiredUpdateRate

    def SetStillUpdateRate(self, rate):
        """Mirrors the method with the same name in
        vtkRenderWindowInteractor."""
        self._StillUpdateRate = rate

    def GetStillUpdateRate(self):
        """Mirrors the method with the same name in
        vtkRenderWindowInteractor."""
        return self._StillUpdateRate

    def GetZoomFactor(self):
        return self._CurrentZoom

    def GetRenderWindow(self):
        return self._RenderWindow

    def GetPicker(self):
        return self._Picker

    def Render(self):
        if (self._CurrentLight):
            light = self._CurrentLight
            light.SetPosition(self._CurrentCamera.GetPosition())
            light.SetFocalPoint(self._CurrentCamera.GetFocalPoint())

        self._RenderWindow.Render()

    def UpdateRenderer(self,x,y):
        """
        UpdateRenderer will identify the renderer under the mouse and set
        up _CurrentRenderer, _CurrentCamera, and _CurrentLight.
        """
        windowX = self.width()
        windowY = self.height()

        renderers = self._RenderWindow.GetRenderers()
        numRenderers = renderers.GetNumberOfItems()

        self._CurrentRenderer = None
        renderers.InitTraversal()
        for i in range(0,numRenderers):
            renderer = renderers.GetNextItem()
            vx,vy = (0,0)
            if (windowX > 1):
                vx = float(x)/(windowX-1)
            if (windowY > 1):
                vy = (windowY-float(y)-1)/(windowY-1)
            (vpxmin,vpymin,vpxmax,vpymax) = renderer.GetViewport()

            if (vx >= vpxmin and vx <= vpxmax and
                vy >= vpymin and vy <= vpymax):
                self._CurrentRenderer = renderer
                self._ViewportCenterX = float(windowX)*(vpxmax-vpxmin)/2.0\
                                        +vpxmin
                self._ViewportCenterY = float(windowY)*(vpymax-vpymin)/2.0\
                                        +vpymin
                self._CurrentCamera = self._CurrentRenderer.GetActiveCamera()
                lights = self._CurrentRenderer.GetLights()
                lights.InitTraversal()
                self._CurrentLight = lights.GetNextItem()
                break

        self._LastX = x
        self._LastY = y

    def GetCurrentRenderer(self):
        return self._CurrentRenderer

    def Rotate(self,x,y):
        if self._CurrentRenderer:

            self._CurrentCamera.Azimuth(self._LastX - x)
            self._CurrentCamera.Elevation(y - self._LastY)
            self._CurrentCamera.OrthogonalizeViewUp()

            self._LastX = x
            self._LastY = y

            self._CurrentRenderer.ResetCameraClippingRange()
            self.Render()

    def Pan(self,x,y):
        if self._CurrentRenderer:

            renderer = self._CurrentRenderer
            camera = self._CurrentCamera
            (pPoint0,pPoint1,pPoint2) = camera.GetPosition()
            (fPoint0,fPoint1,fPoint2) = camera.GetFocalPoint()

            if (camera.GetParallelProjection()):
                renderer.SetWorldPoint(fPoint0,fPoint1,fPoint2,1.0)
                renderer.WorldToDisplay()
                fx,fy,fz = renderer.GetDisplayPoint()
                renderer.SetDisplayPoint(fx-x+self._LastX,
                                         fy+y-self._LastY,
                                         fz)
                renderer.DisplayToWorld()
                fx,fy,fz,fw = renderer.GetWorldPoint()
                camera.SetFocalPoint(fx,fy,fz)

                renderer.SetWorldPoint(pPoint0,pPoint1,pPoint2,1.0)
                renderer.WorldToDisplay()
                fx,fy,fz = renderer.GetDisplayPoint()
                renderer.SetDisplayPoint(fx-x+self._LastX,
                                         fy+y-self._LastY,
                                         fz)
                renderer.DisplayToWorld()
                fx,fy,fz,fw = renderer.GetWorldPoint()
                camera.SetPosition(fx,fy,fz)

            else:
                (fPoint0,fPoint1,fPoint2) = camera.GetFocalPoint()
                # Specify a point location in world coordinates
                renderer.SetWorldPoint(fPoint0,fPoint1,fPoint2,1.0)
                renderer.WorldToDisplay()
                # Convert world point coordinates to display coordinates
                dPoint = renderer.GetDisplayPoint()
                focalDepth = dPoint[2]

                aPoint0 = self._ViewportCenterX + (x - self._LastX)
                aPoint1 = self._ViewportCenterY - (y - self._LastY)

                renderer.SetDisplayPoint(aPoint0,aPoint1,focalDepth)
                renderer.DisplayToWorld()

                (rPoint0,rPoint1,rPoint2,rPoint3) = renderer.GetWorldPoint()
                if (rPoint3 != 0.0):
                    rPoint0 = rPoint0/rPoint3
                    rPoint1 = rPoint1/rPoint3
                    rPoint2 = rPoint2/rPoint3

                camera.SetFocalPoint((fPoint0 - rPoint0) + fPoint0,
                                     (fPoint1 - rPoint1) + fPoint1,
                                     (fPoint2 - rPoint2) + fPoint2)

                camera.SetPosition((fPoint0 - rPoint0) + pPoint0,
                                   (fPoint1 - rPoint1) + pPoint1,
                                   (fPoint2 - rPoint2) + pPoint2)

            self._LastX = x
            self._LastY = y

            self.Render()

    def Zoom(self,x,y):
        if self._CurrentRenderer:

            renderer = self._CurrentRenderer
            camera = self._CurrentCamera

            zoomFactor = math.pow(1.02,(0.5*(self._LastY - y)))
            self._CurrentZoom = self._CurrentZoom * zoomFactor

            if camera.GetParallelProjection():
                parallelScale = camera.GetParallelScale()/zoomFactor
                camera.SetParallelScale(parallelScale)
            else:
                camera.Dolly(zoomFactor)
                renderer.ResetCameraClippingRange()

            self._LastX = x
            self._LastY = y

            self.Render()

    def Reset(self,x,y):
        if self._CurrentRenderer:
            self._CurrentRenderer.ResetCamera()

        self.Render()

    def Wireframe(self):
        actors = self._CurrentRenderer.GetActors()
        numActors = actors.GetNumberOfItems()
        actors.InitTraversal()
        for i in range(0,numActors):
            actor = actors.GetNextItem()
            actor.GetProperty().SetRepresentationToWireframe()

        self.Render()

    def Surface(self):
        actors = self._CurrentRenderer.GetActors()
        numActors = actors.GetNumberOfItems()
        actors.InitTraversal()
        for i in range(0,numActors):
            actor = actors.GetNextItem()
            actor.GetProperty().SetRepresentationToSurface()

        self.Render()

    def PickActor(self,x,y):
        if self._CurrentRenderer:

            renderer = self._CurrentRenderer
            picker = self._Picker

            windowY = self.height()
            picker.Pick(x,(windowY - y - 1),0.0,renderer)
            actor = picker.GetActor()

            if (self._PickedActor != None and
                self._PrePickedProperty != None):
                self._PickedActor.SetProperty(self._PrePickedProperty)
                # release hold of the property
                self._PrePickedProperty.UnRegister(self._PrePickedProperty)
                self._PrePickedProperty = None

            if (actor != None):
                self._PickedActor = actor
                self._PrePickedProperty = self._PickedActor.GetProperty()
                # hold onto the property
                self._PrePickedProperty.Register(self._PrePickedProperty)
                self._PickedActor.SetProperty(self._PickedProperty)

            self.Render()

#----------------------------------------------------------------------------
def QVTKRenderWidgetConeExample():
    """Like it says, just a simple example
    """
    # every QT app needs an app
    app = QApplication(['QVTKRenderWidget'])

    # create the widget
    widget = QVTKRenderWidget()

    ren = vtk.vtkRenderer()
    widget.GetRenderWindow().AddRenderer(ren)

    cone = vtk.vtkConeSource()
    cone.SetResolution(8)

    coneMapper = vtk.vtkPolyDataMapper()
    coneMapper.SetInputConnection(cone.GetOutputPort())

    coneActor = vtk.vtkActor()
    coneActor.SetMapper(coneMapper)

    ren.AddActor(coneActor)

    # show the widget
    widget.show()
    # close the application when window is closed
    qApp.setMainWidget(widget)
    # start event processing
    app.exec_loop()

if __name__ == "__main__":
    QVTKRenderWidgetConeExample()

