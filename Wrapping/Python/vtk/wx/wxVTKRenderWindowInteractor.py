"""

A VTK RenderWindowInteractor widget for wxPython.  Note that wxPython
comes with its own wxVTKRenderWindow in wxPython.lib.vtk.  Try both
and see which one works better for you.  

Find wxPython info at http://wxPython.org

Created by Prabhu Ramachandran, April 2002
Based on wxVTKRenderWindow.py
"""

"""
Please see the example at the end of this file.

----------------------------------------
Creation:

 wxVTKRenderWindowInteractor(parent, ID, stereo=0, [wx keywords]):
 
 You should create a wxPySimpleApp() or some other wx**App before
 creating the window.

Behaviour:

 Uses __getattr__ to make the wxVTKRenderWindowInteractor behave just
 like a vtkGenericRenderWindowInteractor.

----------------------------------------

"""

# import usual libraries
import math, os, sys
from wxPython.wx import *
import vtk

# wxPython 2.4.0.4 and newer prefers the use of True and False, standard
# booleans in Python 2.2 but not earlier.  Here we define these values if
# they don't exist so that we can use True and False in the rest of the
# code.  At the time of this writing, that happens exactly ONCE in
# CreateTimer()
try:
    True
except NameError:
    True = 1
    False = 0

# a few configuration items, see what works best on your system

# Use wxGLCanvas as base class instead of wxWindow.
# This is sometimes necessary under wxGTK or the image is blank.
# (in wxWindows 2.3.1 and earlier, the GLCanvas had scroll bars)
try:
    WX_USE_GL_CANVAS
except NameError:
    if wxPlatform == '__WXMSW__':
        WX_USE_GLCANVAS = 0
    else:
        WX_USE_GLCANVAS = 1
        
# Keep capturing mouse after mouse is dragged out of window
# (in wxGTK 2.3.2 there is a bug that keeps this from working,
# but it is only relevant in wxGTK if there are multiple windows)
try:
    WX_USE_X_CAPTURE
except NameError:
    if wxPlatform == '__WXMSW__': 
        WX_USE_X_CAPTURE = 1
    else:
        WX_USE_X_CAPTURE = 0

# end of configuration items


if WX_USE_GLCANVAS:
    from wxPython.glcanvas import *
    baseClass = wxGLCanvas
else:
    baseClass = wxWindow

class EventTimer(wxTimer):
    def __init__(self, iren):
        wxTimer.__init__(self)
        self.iren = iren
    def Notify(self):
        self.iren.TimerEvent()

class wxVTKRenderWindowInteractor(baseClass):
    """
    A wxRenderWindow for wxPython.
    Use GetRenderWindow() to get the vtkRenderWindow.
    Create with the keyword stereo=1 in order to
    generate a stereo-capable window.
    """
    def __init__(self, parent, ID, *args, **kw):

        # private attributes
        self.__RenderWhenDisabled = 0

        self.__RenderWhenDisabled = 0

        # First do special handling of some keywords:
        # stereo, position, size, width, height, style
        
        stereo = 0
        
        if kw.has_key('stereo'):
            if kw['stereo']:
                stereo = 1
            del kw['stereo']

        position = wxDefaultPosition

        if kw.has_key('position'):
            position = kw['position']
            del kw['position']

        size = wxDefaultSize

        if kw.has_key('size'):
            size = kw['size']
            del kw['size']
        
        if kw.has_key('width') and kw.has_key('height'):
            size = (kw['width'], kw['height'])
            del kw['width']
            del kw['height']

        # wxWANTS_CHARS says to give us e.g. TAB
        # wxNO_FULL_REPAINT_ON_RESIZE cuts down resize flicker under GTK
        style = wxWANTS_CHARS | wxNO_FULL_REPAINT_ON_RESIZE

        if kw.has_key('style'):
            style = style | kw['style']
            del kw['style']

        # the enclosing frame must be shown under GTK or the windows
        #  don't connect together properly
        l = []
        p = parent
        while p: # make a list of all parents
            l.append(p)
            p = p.GetParent()
        l.reverse() # sort list into descending order
        for p in l:
            p.Show(1)

        # initialize the wxWindow
        baseClass.__init__(self, parent, ID, position, size, style)

        # create the RenderWindow and initialize it
        self._Iren = vtk.vtkGenericRenderWindowInteractor()
        self._Iren.SetRenderWindow( vtk.vtkRenderWindow() )
        self._Iren.AddObserver('CreateTimerEvent', self.CreateTimer)
        self._Iren.AddObserver('DestroyTimerEvent', self.DestroyTimer)

        try:
            self._Iren.GetRenderWindow().SetSize(size.width, size.height)
        except AttributeError:
            self._Iren.GetRenderWindow().SetSize(size[0], size[1])
        if stereo:
            self._Iren.GetRenderWindow().StereoCapableWindowOn()
            self._Iren.GetRenderWindow().SetStereoTypeToCrystalEyes()

        self.__handle = None
        self._ActiveButton = 0

        self.BindEvents()

    def BindEvents(self):
        # refresh window by doing a Render
        EVT_PAINT(self, self.OnPaint)
        # turn off background erase to reduce flicker
        EVT_ERASE_BACKGROUND(self, lambda e: None)
        
        # Bind the events to the event converters
        EVT_RIGHT_DOWN(self, self.OnButtonDown)
        EVT_LEFT_DOWN(self, self.OnButtonDown)
        EVT_MIDDLE_DOWN(self, self.OnButtonDown)
        EVT_RIGHT_UP(self, self.OnButtonUp)
        EVT_LEFT_UP(self, self.OnButtonUp)
        EVT_MIDDLE_UP(self, self.OnButtonUp)
        EVT_MOTION(self, self.OnMotion)

        EVT_ENTER_WINDOW(self, self.OnEnter)
        EVT_LEAVE_WINDOW(self, self.OnLeave)

        # If we use EVT_KEY_DOWN instead of EVT_CHAR, capital versions
        # of all characters are always returned.  EVT_CHAR also performs
        # other necessary keyboard-dependent translations.
        EVT_CHAR(self, self.OnKeyDown)
        EVT_KEY_UP(self, self.OnKeyUp)
        
        EVT_SIZE(self, self.OnSize)

    def __getattr__(self, attr):        
        """Makes the object behave like a
        vtkGenericRenderWindowInteractor"""
        if attr == '__vtk__':
            return lambda t=self._Iren: t
        elif hasattr(self._Iren, attr):
            return getattr(self._Iren, attr)
        else:
            raise AttributeError, self.__class__.__name__ + \
                  " has no attribute named " + attr

    def CreateTimer(self, obj, evt):
        t = EventTimer(self)
        t.Start(10, True)

    def DestroyTimer(self, obj, evt):
        """The timer is a one shot timer so will expire automatically."""
        return 1

    def OnPaint(self,event):
        dc = wxPaintDC(self)
        # Tell the RenderWindow to render inside the wxWindow.
        if not self.__handle:
            self.__handle = self.GetHandle()
            self._Iren.GetRenderWindow().SetWindowInfo(str(self.__handle))
        self.Render()

    def OnSize(self,event):
        try:
            width, height = event.GetSize()
        except:
            width = event.GetSize().width
            height = event.GetSize().height
        self._Iren.SetSize(width, height)
        self._Iren.ConfigureEvent()
        # this will check for __handle
        self.Render()

    def OnMotion(self, event):
        self._Iren.SetEventInformationFlipY(event.GetX(), event.GetY(),
                                            event.ControlDown(), 
					    event.ShiftDown(), 
					    chr(0), 0, None)
        self._Iren.MouseMoveEvent()

    def OnEnter(self,event):
        self._Iren.SetEventInformationFlipY(event.GetX(), event.GetY(),
                                            event.ControlDown(), 
					    event.ShiftDown(), 
					    chr(0), 0, None)
        self._Iren.EnterEvent()
        
    def OnLeave(self,event):
        self._Iren.SetEventInformationFlipY(event.GetX(), event.GetY(),
                                            event.ControlDown(), 
					    event.ShiftDown(), 
					    chr(0), 0, None)
        self._Iren.LeaveEvent()        

    def OnButtonDown(self,event):
        ctrl, shift = event.ControlDown(), event.ShiftDown()
        self._Iren.SetEventInformationFlipY(event.GetX(), event.GetY(),
                                            ctrl, shift, chr(0), 0, None)

        self._ActiveButton = 0
        if event.RightDown():
            self._Iren.RightButtonPressEvent()
            self._ActiveButton = 'Right'
        elif event.LeftDown():
            self._Iren.LeftButtonPressEvent()
            self._ActiveButton = 'Left'
        elif event.MiddleDown():
            self._Iren.MiddleButtonPressEvent()
            self._ActiveButton = 'Middle'

        # save the button and capture mouse until the button is released
        if self._ActiveButton and WX_USE_X_CAPTURE:
            self.CaptureMouse()

    def OnButtonUp(self,event):
        ctrl, shift = event.ControlDown(), event.ShiftDown()
        self._Iren.SetEventInformationFlipY(event.GetX(), event.GetY(),
                                            ctrl, shift, chr(0), 0, None)

        if self._ActiveButton == 'Right':
            self._Iren.RightButtonReleaseEvent()
        elif self._ActiveButton == 'Left':
            self._Iren.LeftButtonReleaseEvent()
        elif self._ActiveButton == 'Middle':
            self._Iren.MiddleButtonReleaseEvent()

        # if the ActiveButton is realeased, then release mouse capture
        if self._ActiveButton and WX_USE_X_CAPTURE:
            self.ReleaseMouse()

    def OnKeyDown(self,event):
        ctrl, shift = event.ControlDown(), event.ShiftDown()
        keycode, keysym = event.GetKeyCode(), None
        key = chr(0)
        if keycode < 256:
            key = chr(keycode)

        self._Iren.SetEventInformationFlipY(event.GetX(), event.GetY(),
                                            ctrl, shift, key, 0,
                                            keysym)
        self._Iren.KeyPressEvent()
        self._Iren.CharEvent()

    def OnKeyUp(self,event):
        ctrl, shift = event.ControlDown(), event.ShiftDown()
        keycode, keysym = event.GetKeyCode(), None
        key = chr(0)
        if keycode < 256:
            key = chr(keycode)

        self._Iren.SetEventInformationFlipY(event.GetX(), event.GetY(),
                                            ctrl, shift, key, 0,
                                            keysym)
        self._Iren.KeyReleaseEvent()

    def GetRenderWindow(self):
        return self._Iren.GetRenderWindow()

    def Render(self):
        RenderAllowed = 1
        
        if not self.__RenderWhenDisabled:
            # the user doesn't want us to render when the toplevel frame
            # is disabled - first find the top level parent
            topParent = wxGetTopLevelParent(self)
            if topParent:
                # if it exists, check whether it's enabled
                # if it's not enabeld, RenderAllowed will be false
                RenderAllowed = topParent.IsEnabled()
            
        if RenderAllowed:
            if self.__handle and self.__handle == self.GetHandle():
                self._Iren.GetRenderWindow().Render()

            elif self.GetHandle():
                # this means the user has reparented us; let's adapt to the
                # new situation by doing the WindowRemap dance
                self._Iren.GetRenderWindow().SetNextWindowInfo(str(self.GetHandle()))
                self._Iren.GetRenderWindow().WindowRemap()
                # store the new situation
                self.__handle = self.GetHandle()
                self._Iren.GetRenderWindow().Render()

    def SetRenderWhenDisabled(self, newValue):
        """Change value of __RenderWhenDisabled ivar.

        If __RenderWhenDisabled is false (the default), this widget will not
        call Render() on the RenderWindow if the top level frame (i.e. the
        containing frame) has been disabled.

        This prevents recursive rendering during wxSafeYield() calls.
        wxSafeYield() can be called during the ProgressMethod() callback of
        a VTK object to have progress bars and other GUI elements updated -
        it does this by disabling all windows (disallowing user-input to
        prevent re-entrancy of code) and then handling all outstanding
        GUI events.
        
        However, this often triggers an OnPaint() method for wxVTKRWIs,
        resulting in a Render(), resulting in Update() being called whilst
        still in progress.
        """
        self.__RenderWhenDisabled = bool(newValue)

    def SetRenderWhenDisabled(self, newValue):
        """Change value of __RenderWhenDisabled ivar.

        If __RenderWhenDisabled is false (the default), this widget will not
        call Render() on the RenderWindow if the top level frame (i.e. the
        containing frame) has been disabled.

        This prevents recursive rendering during wxSafeYield() calls.
        wxSafeYield() can be called during the ProgressMethod() callback of
        a VTK object to have progress bars and other GUI elements updated -
        it does this by disabling all windows (disallowing user-input to
        prevent re-entrancy of code) and then handling all outstanding
        GUI events.
        
        However, this often triggers an OnPaint() method for wxVTKRWIs,
        resulting in a Render(), resulting in Update() being called whilst
        still in progress.
        """
        self.__RenderWhenDisabled = bool(newValue)


#--------------------------------------------------------------------  
def wxVTKRenderWindowInteractorConeExample():
    """Like it says, just a simple example
    """
    # every wx app needs an app
    app = wxPySimpleApp()

    # create the widget
    frame = wxFrame(None, -1, "wxRenderWindow", size=wxSize(400,400))
    widget = wxVTKRenderWindowInteractor(frame, -1)
    widget.Initialize()
    widget.Start()
    widget.AddObserver("ExitEvent", lambda o,e,f=frame: f.Close())

    ren = vtk.vtkRenderer()
    widget.GetRenderWindow().AddRenderer(ren)

    cone = vtk.vtkConeSource()
    cone.SetResolution(8)
    
    coneMapper = vtk.vtkPolyDataMapper()
    coneMapper.SetInput(cone.GetOutput())
    
    coneActor = vtk.vtkActor()
    coneActor.SetMapper(coneMapper)

    ren.AddActor(coneActor)

    # show the window
    
    frame.Show(1)

    app.MainLoop()

if __name__ == "__main__":
    wxVTKRenderWindowInteractorConeExample()

