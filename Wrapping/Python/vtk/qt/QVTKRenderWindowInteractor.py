"""
A simple VTK input file for PyQt, the qt bindings for python.
See http://www.trolltech.com for qt documentation, and
http://www.thekompany.com for the qt python bindings.

This class is based on the vtkGenericRenderWindowInteractor and is
therefore fairly powerful.  It should also play nicely with the
vtk3DWidget code.

Created by Prabhu Ramachandran, May 2002
Based on David Gobbi's QVTKRenderWidget.py
"""

"""
This class works only with the UNIX version of Qt.
It does not work under the Win32 version of Qt.

Depending on the OpenGL graphics drivers, it may not
be possible to have more than one QVTKRenderWidget
per application.

In short, this class is experimental.
"""

import math, os, sys
import qt
import vtk

class QVTKRenderWindowInteractor(qt.QWidget):

    """ A QVTKRenderWindowInteractor for Python and Qt.  Uses a
    vtkGenericRenderWindowInteractor to handle the interactions.  Use
    GetRenderWindow() to get the vtkRenderWindow.  Create with the
    keyword stereo=1 in order to generate a stereo-capable window.
    """
    
    def __init__(self, parent=None, name=None, *args, **kw):

        # the current button
        self._ActiveButton = 0

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
        apply(qt.QWidget.__init__, (self,parent,name) + args, kw)

        if rw: # user-supplied render window
            self._RenderWindow = rw
        else:
            self._RenderWindow = vtk.vtkRenderWindow()

        if stereo: # stereo mode
            self._RenderWindow.StereoCapableWindowOn()
            self._RenderWindow.SetStereoTypeToCrystalEyes()

        self._Iren = vtk.vtkGenericRenderWindowInteractor()
        self._Iren.SetRenderWindow(self._RenderWindow)

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
                self._RenderWindow.SetWindowInfo(str(self.winId()))
                self.__connected = 1

        self._Timer = qt.QTimer(self, 'timer handler')
        self.connect(self._Timer, qt.SIGNAL('timeout()'),
                     self.TimerEvent)

        self._Iren.AddObserver('CreateTimerEvent', self.CreateTimer)
        self._Iren.AddObserver('DestroyTimerEvent', self.DestroyTimer)
        
    def __getattr__(self, attr):        
        """Makes the object behave like a
        vtkGenericRenderWindowInteractor"""
        if attr == '__vtk__':
            return lambda t=self._Iren: t
        elif hasattr(self._Iren, attr):
            return getattr(self._Iren, attr)
        elif hasattr(qt.QWidget, attr):
            return getattr(self.sipThis, attr)
        else:
            raise AttributeError, self.__class__.__name__ + \
                  " has no attribute named " + attr

    def CreateTimer(self, obj, evt):
        self._Timer.start(10)

    def DestroyTimer(self, obj, evt):
        self._Timer.stop()
        return 1

    def TimerEvent(self):
        self._Iren.TimerEvent()

    def paintEvent(self,ev):
        if self.isVisible():
            if self.__connected == 0:
                size = self.size()
                self._Iren.SetSize(size.width(),size.height())
                self._RenderWindow.SetWindowInfo(str(self.winId()))
                self._Iren.ConfigureEvent()
                self.__connected = 1
        if self.__connected:
            self.Render()

    def resizeEvent(self,ev):
        size = self.size()
        self._Iren.SetSize(size.width(),size.height())
        self._Iren.ConfigureEvent()
        self.repaint()

    def enterEvent(self,ev):
        if not self.hasFocus():
            self.__oldFocus = self.focusWidget()
            self.setFocus()
        self._Iren.SetEventInformationFlipY(self.__saveX, self.__saveY,
                                            0, 0, chr(0), 0, None)
        self._Iren.EnterEvent()

    def leaveEvent(self,ev):
        if (self.__saveState & 0x7) == 0 and self.__oldFocus:
            self.__oldFocus.setFocus()
            self.__oldFocus = None
        self._Iren.SetEventInformationFlipY(self.__saveX, self.__saveY,
                                            0, 0, chr(0), 0, None)
        self._Iren.LeaveEvent()

    def _GetCtrlShift(self, ev):
        ctrl, shift = 0, 0
        if (ev.state() & 8):
            shift = 1
        if (ev.state() & 16):
            ctrl = 1
        return ctrl, shift

    def mousePressEvent(self,ev):
        ctrl, shift = self._GetCtrlShift(ev)
        self._Iren.SetEventInformationFlipY(ev.x(), ev.y(),
                                            ctrl, shift, chr(0), 0, None)

        self._ActiveButton = 0
        if ev.button() == 1:
            self._Iren.LeftButtonPressEvent()
            self._ActiveButton = 'Left'
        elif ev.button() == 2:
            self._Iren.RightButtonPressEvent()
            self._ActiveButton = 'Right'
        elif ev.button() == 4:
            self._Iren.MiddleButtonPressEvent()
            self._ActiveButton = 'Middle'

    def mouseReleaseEvent(self,ev):
        ctrl, shift = self._GetCtrlShift(ev)
        self._Iren.SetEventInformationFlipY(ev.x(), ev.y(),
                                            ctrl, shift, chr(0), 0, None)
        
        if self._ActiveButton == 'Right':
            self._Iren.RightButtonReleaseEvent()
        elif self._ActiveButton == 'Left':
            self._Iren.LeftButtonReleaseEvent()
        elif self._ActiveButton == 'Middle':
            self._Iren.MiddleButtonReleaseEvent()

    def mouseMoveEvent(self,ev):
        self.__saveState = ev.state()
        self.__saveX = ev.x()
        self.__saveY = ev.y()
        self._Iren.SetEventInformationFlipY(ev.x(), ev.y(),
                                            0, 0, chr(0), 0, None)
        self._Iren.MouseMoveEvent()

    def keyPressEvent(self,ev):
        ctrl, shift = self._GetCtrlShift(ev)
        key = chr(0)
        if ev.key() < 256:
            key = chr(ev.key())

        self._Iren.SetEventInformationFlipY(self.__saveX, self.__saveY,
                                            ctrl, shift, key, 0,
                                            None)
        self._Iren.KeyPressEvent()
        self._Iren.CharEvent()
        
    def keyReleaseEvent(self,ev):
        ctrl, shift = self._GetCtrlShift(ev)
        key = chr(0)
        if ev.key() < 256:
            key = chr(ev.key())

        self._Iren.SetEventInformationFlipY(self.__saveX, self.__saveY,
                                            ctrl, shift, key, 0,
                                            None)
        self._Iren.KeyReleaseEvent()

    def GetRenderWindow(self):
        return self._RenderWindow

    def Render(self):
        self._RenderWindow.Render()

#----------------------------------------------------------------------------  
def QVTKRenderWidgetConeExample():
    """Like it says, just a simple example
    """
    # every QT app needs an app
    app = qt.QApplication(['QVTKRenderWindowInteractor'])

    # create the widget
    widget = QVTKRenderWindowInteractor()
    widget.Initialize()
    widget.Start()
    # if you dont want the 'q' key to exit comment this.
    widget.SetExitMethod(app.quit)

    ren = vtk.vtkRenderer()
    widget.GetRenderWindow().AddRenderer(ren)

    cone = vtk.vtkConeSource()
    cone.SetResolution(8)
    
    coneMapper = vtk.vtkPolyDataMapper()
    coneMapper.SetInput(cone.GetOutput())
    
    coneActor = vtk.vtkActor()
    coneActor.SetMapper(coneMapper)

    ren.AddActor(coneActor)

    # show the widget
    widget.show()
    # close the application when window is closed
    app.setMainWidget(widget)
    # start event processing
    app.exec_loop()
    
if __name__ == "__main__":
    QVTKRenderWidgetConeExample()

