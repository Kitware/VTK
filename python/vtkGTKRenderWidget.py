# Id
#
# Description:
#
# Provides classes called GtkVtkRenderWindowBase and
# GtkVtkRenderWindow that are used to embed a vtkRenderWindow inside a
# GTK widget.  The GtkVtkRenderWindow is very similar to the
# vtkTkRenderWidget class.  For the interaction I have liberally
# 'stolen' code from the vtkRenderWidget.py.  The
# GtkVtkRenderWindowBase provides the abstraction necessary for
# someone to use their own interaction behaviour.  The method names
# are similar to those in vtkInteractorStyle.h.
#
# There is a global DEBUG flag that is useful if you want to either
# debug the code or look at the event generation.  This can be
# instructive.
#
# Created:
#
# March, 2001 by Prabhu Ramachandran <prabhu@aero.iitm.ernet.in>
#
# Bugs:
#
# (*) There is a focus related problem.  Tkinter has a focus object
# that handles focus events.  I dont know of an equivalent object
# under GTK.  So, when an 'enter_notify_event' is received on the
# GtkVtkRenderWindow I grab the focus but I dont know what to do when
# I get a 'leave_notify_event'.
#
# (*) I use a hack to enable proper event handling.  This is done
# because I use the SetParentInfo method to set the XID of the
# vtkRenderWidget.  This creates problems and doesnt generate proper
# expose events.  The hack fixes this.

import gtk
import GDK
from GtkExtra import message_box
import vtkpython
import math

DEBUG = 0
def debug (msg):
    if DEBUG:
        print msg

class GtkVtkRenderWindowBase (gtk.GtkViewport):

    """ A base class that enables one to embed a vtkRenderWindow into
    a pyGTK widget.  This class embeds the RenderWindow correctly.
    Provided are some empty methods that can be overloaded to provide
    a user defined interaction behaviour.  The event handling
    functions have names that are somewhat similar to the ones in the
    vtkInteractorStyle class included with VTK. """

    def __init__ (self, *args):
        l = list (args)
        l.insert (0, self)
        apply (gtk.GtkViewport.__init__, l)
        self._RenderWindow = vtkpython.vtkRenderWindow ()

        # This box is necessary to eliminate flicker and enable proper
        # event handling.  This is because of the SetParentInfo that
        # is used to set the XID of the window and embed the widget
        # appropriately.        
        self.box = gtk.GtkEventBox ()
        self.box.show ()
        self.add (self.box)

        # private attributes
        self.__InExpose = 0
        self.__Created = 0

        self.ConnectSignals ()
        
        # need this to be able to handle key_press events.
        self.box.set_flags (gtk.CAN_FOCUS)
        # default size
        self.set_usize (300, 300)
        
    def ConnectSignals (self):
        debug ("In GtkVtkRenderWindowBase::ConnectSignals")
        self.connect ("realize", self.OnRealize)
        self.connect ("expose_event", self.OnExpose)
        self.box.connect_after ("size_allocate", self.OnConfigure)
        self.box.connect ("button_press_event", self.OnButtonDown)
        self.box.connect ("button_release_event", self.OnButtonUp)
        self.box.connect ("motion_notify_event", self.OnMouseMove)
        self.box.connect ("enter_notify_event", self.OnEnter)
        self.box.connect ("leave_notify_event", self.OnLeave)
        self.box.connect ("key_press_event", self.OnKeyPress)
        self.connect ("destroy", self.OnDestroy) 
        #self.connect ("delete_event", self.OnDestroy)
        self.box.add_events (GDK.EXPOSURE_MASK| GDK.BUTTON_PRESS_MASK |
                             GDK.BUTTON_RELEASE_MASK |
                             GDK.KEY_PRESS_MASK |
                             GDK.POINTER_MOTION_MASK |
                             GDK.POINTER_MOTION_HINT_MASK |
                             GDK.ENTER_NOTIFY_MASK | GDK.LEAVE_NOTIFY_MASK)
        
    def GetRenderWindow (self):
        return self._RenderWindow

    def GetRenderer (self):
        self._RenderWindow.GetRenderers ().InitTraversal ()
        return self._RenderWindow.GetRenderers ().GetNextItem ()

    def OnRealize (self, *args):
        debug ("In GtkVtkRenderWindowBase::OnRealize")
        if self.__Created == 0:
            # you can't get the xid without the window being realized.
            self.box.realize ()
            win_id = str (self.box.get_window ().xid)
            try:
                self._RenderWindow.SetParentInfo (win_id)
            except AttributeError:
                self._RenderWindow.SetWindowInfo (win_id)                
                msg = "Warning:\n "\
                      "Unable to call vtkRenderWindow.SetParentInfo\n\n"\
                      "Using the SetWindowInfo method instead.  This\n"\
                      "is likely to cause a lot of flicker when\n"\
                      "rendering in the vtkRenderWindow.  Please\n"\
                      "use a recent Nightly VTK release (later than\n"\
                      "March 10 2001) to eliminate this problem."
                message_box ("Warning", msg, ('Continue', ))
            self.__Created = 1
    
    def OnConfigure (self, widget, allocation):
        debug ("In GtkVtkRenderWindowBase::OnConfigure")
        sz = self._RenderWindow.GetSize ()
        x, y, width, height = widget.get_allocation()
        if (width != sz[0]) or (height != sz[1]):
            self._RenderWindow.SetSize (width, height)
            while gtk.events_pending ():
                gtk.mainiteration ()
            if self.__Created:
                self._RenderWindow.Render ()

    def OnExpose (self, *args):
        debug ("In GtkVtkRenderWindowBase::OnExpose")
        if (not self.__InExpose):
            self.__InExpose = 1
            self._RenderWindow.Render ()
            self.__InExpose = 0
        return gtk.TRUE

    def OnDestroy (self, event=None):
        debug ("In GtkVtkRenderWindowBase::OnDestroy")
        self.box.hide ()
        self.hide ()
        self.box.destroy ()
        self.destroy ()

    def OnButtonDown (self, wid, event):
        "Mouse button pressed."
        pass
    
    def OnButtonUp (self, wid, event):
        "Mouse button released."
        pass

    def OnMouseMove (self, wid, event):
        "Mouse has moved."
        pass

    def OnEnter (self, wid, event):
        "Entering the vtkRenderWindow."
        pass

    def OnLeave (self, wid, event):
        "Leaving the vtkRenderWindow."
        pass
    
    def OnKeyPress (self, wid, event):
        "Key pressed."
        pass

    def OnKeyRelease (self, wid, event):
        "Key released."
        pass


class GtkVtkRenderWindow (GtkVtkRenderWindowBase):

    """ An example of a fully functional GtkVtkRenderWindow that is
    based on the vtkRenderWidget.py provided with the VTK sources."""

    def __init__ (self, *args):
        l = list (args)
        l.insert (0, self)
        apply (GtkVtkRenderWindowBase.__init__, l)
        
        self._CurrentRenderer = None
        self._CurrentCamera = None
        self._CurrentZoom = 1.0
        self._CurrentLight = None

        self._ViewportCenterX = 0
        self._ViewportCenterY = 0
        
        self._Picker = vtkpython.vtkCellPicker ()
        self._PickedAssembly = None
        self._PickedProperty = vtkpython.vtkProperty ()
        self._PickedProperty.SetColor (1, 0, 0)
        self._PrePickedProperty = None
        
        self._OldFocus = None

        # these record the previous mouse position
        self._LastX = 0
        self._LastY = 0

    def OnButtonDown (self, wid, event):
        debug ("In GtkVtkRenderWindow::OnButtonDown")
        return self.StartMotion (wid, event)
    
    def OnButtonUp (self, wid, event):
        debug ("In GtkVtkRenderWindow::OnButtonUp")
        return self.EndMotion (wid, event)

    def OnMouseMove (self, wid, event=None):
        debug ("In GtkVtkRenderWindow::OnMouseMove")
        if ((event.state & GDK.BUTTON1_MASK) == GDK.BUTTON1_MASK):
            if ((event.state & GDK.SHIFT_MASK) == GDK.SHIFT_MASK):
                m = self.box.get_pointer ()
                self.Pan (m[0], m[1])
                return gtk.TRUE
            else:
                m = self.box.get_pointer ()
                self.Rotate (m[0], m[1])
                return gtk.TRUE
        elif ((event.state & GDK.BUTTON2_MASK) == GDK.BUTTON2_MASK):
            m = self.box.get_pointer ()
            self.Pan (m[0], m[1])
            return gtk.TRUE
        elif ((event.state & GDK.BUTTON3_MASK) == GDK.BUTTON3_MASK):
            m = self.box.get_pointer ()
            self.Zoom (m[0], m[1])
            return gtk.TRUE
        else:
            return gtk.FALSE

    def OnEnter (self, wid, event=None):
        debug ("In GtkVtkRenderWindow::OnEnter")
        self.box.grab_focus ()
        w = self.box.get_pointer ()
        self.UpdateRenderer (w[0], w[1])
        return gtk.TRUE

    def OnLeave (self, wid, event):
        debug ("In GtkVtkRenderWindow::OnLeave")
        return gtk.TRUE

    def OnKeyPress (self, wid, event=None):
        debug ("In GtkVtkRenderWindow::OnKeyPress")
        if (event.keyval == GDK.r) or (event.keyval == GDK.R):
            self.Reset ()
            return gtk.TRUE
        elif (event.keyval == GDK.w) or (event.keyval == GDK.W):
            self.Wireframe ()
            return gtk.TRUE
        elif (event.keyval == GDK.s) or (event.keyval == GDK.S):
            self.Surface ()
            return gtk.TRUE
        elif (event.keyval == GDK.p) or (event.keyval == GDK.P):
            m = self.box.get_pointer ()
            self.PickActor (m[0], m[1])
            return gtk.TRUE
        else:
            return gtk.FALSE

    def GetZoomFactor (self):
        return self._CurrentZoom

    def SetZoomFactor (self, zf):
        self._CurrentZoom = zf

    def GetPicker (self):
        return self._Picker

    def Render (self):
        if (self._CurrentLight):
            light = self._CurrentLight
            light.SetPosition (self._CurrentCamera.GetPosition ())
            light.SetFocalPoint (self._CurrentCamera.GetFocalPoint ())

        self._RenderWindow.Render ()

    def UpdateRenderer (self,x,y):
        """
        UpdateRenderer will identify the renderer under the mouse and set
        up _CurrentRenderer, _CurrentCamera, and _CurrentLight.
        """
        windowX = self.box.get_window ().width
        windowY = self.box.get_window ().height

        renderers = self._RenderWindow.GetRenderers ()
        numRenderers = renderers.GetNumberOfItems ()

        self._CurrentRenderer = None
        renderers.InitTraversal ()
        for i in range (0,numRenderers):
            renderer = renderers.GetNextItem ()
            vx,vy = (0,0)
            if (windowX > 1):
                vx = float (x)/(windowX-1)
            if (windowY > 1):
                vy = (windowY-float (y)-1)/(windowY-1)
            (vpxmin,vpymin,vpxmax,vpymax) = renderer.GetViewport ()
            
            if (vx >= vpxmin and vx <= vpxmax and
                vy >= vpymin and vy <= vpymax):
                self._CurrentRenderer = renderer
                self._ViewportCenterX = float (windowX)*(vpxmax-vpxmin)/2.0\
                                        +vpxmin
                self._ViewportCenterY = float (windowY)*(vpymax-vpymin)/2.0\
                                        +vpymin
                self._CurrentCamera = self._CurrentRenderer.GetActiveCamera ()
                lights = self._CurrentRenderer.GetLights ()
                lights.InitTraversal ()
                self._CurrentLight = lights.GetNextItem ()
                break

        self._LastX = x
        self._LastY = y

    def GetCurrentRenderer (self):
        return self._CurrentRenderer
                
    def StartMotion (self, wid, event=None):
        x = event.x
        y = event.y
        self.UpdateRenderer (x,y)
        return gtk.TRUE

    def EndMotion (self, wid, event=None):
        if self._CurrentRenderer:
            self.Render ()
        return gtk.TRUE

    def Rotate (self,x,y):
        if self._CurrentRenderer:
            
            self._CurrentCamera.Azimuth (self._LastX - x)
            self._CurrentCamera.Elevation (y - self._LastY)
            self._CurrentCamera.OrthogonalizeViewUp ()
            
            self._LastX = x
            self._LastY = y
            
            self._CurrentRenderer.ResetCameraClippingRange ()
            self.Render ()

    def Pan (self,x,y):
        if self._CurrentRenderer:
            
            renderer = self._CurrentRenderer
            camera = self._CurrentCamera
            (pPoint0,pPoint1,pPoint2) = camera.GetPosition ()
            (fPoint0,fPoint1,fPoint2) = camera.GetFocalPoint ()

            if (camera.GetParallelProjection ()):
                renderer.SetWorldPoint (fPoint0,fPoint1,fPoint2,1.0)
                renderer.WorldToDisplay ()
                fx,fy,fz = renderer.GetDisplayPoint ()
                renderer.SetDisplayPoint (fx-x+self._LastX,
                                         fy+y-self._LastY,
                                         fz)
                renderer.DisplayToWorld ()
                fx,fy,fz,fw = renderer.GetWorldPoint ()
                camera.SetFocalPoint (fx,fy,fz)

                renderer.SetWorldPoint (pPoint0,pPoint1,pPoint2,1.0)
                renderer.WorldToDisplay ()
                fx,fy,fz = renderer.GetDisplayPoint ()
                renderer.SetDisplayPoint (fx-x+self._LastX,
                                         fy+y-self._LastY,
                                         fz)
                renderer.DisplayToWorld ()
                fx,fy,fz,fw = renderer.GetWorldPoint ()
                camera.SetPosition (fx,fy,fz)
                
            else:
                (fPoint0,fPoint1,fPoint2) = camera.GetFocalPoint ()
                # Specify a point location in world coordinates
                renderer.SetWorldPoint (fPoint0,fPoint1,fPoint2,1.0)
                renderer.WorldToDisplay ()
                # Convert world point coordinates to display coordinates
                dPoint = renderer.GetDisplayPoint ()
                focalDepth = dPoint[2]
                
                aPoint0 = self._ViewportCenterX + (x - self._LastX)
                aPoint1 = self._ViewportCenterY - (y - self._LastY)
                
                renderer.SetDisplayPoint (aPoint0,aPoint1,focalDepth)
                renderer.DisplayToWorld ()
                
                (rPoint0,rPoint1,rPoint2,rPoint3) = renderer.GetWorldPoint ()
                if (rPoint3 != 0.0):
                    rPoint0 = rPoint0/rPoint3
                    rPoint1 = rPoint1/rPoint3
                    rPoint2 = rPoint2/rPoint3

                camera.SetFocalPoint ((fPoint0 - rPoint0) + fPoint0, 
                                     (fPoint1 - rPoint1) + fPoint1,
                                     (fPoint2 - rPoint2) + fPoint2) 
                
                camera.SetPosition ((fPoint0 - rPoint0) + pPoint0, 
                                   (fPoint1 - rPoint1) + pPoint1,
                                   (fPoint2 - rPoint2) + pPoint2)

            self._LastX = x
            self._LastY = y

            self.Render ()

    def Zoom (self,x,y):
        if self._CurrentRenderer:

            renderer = self._CurrentRenderer
            camera = self._CurrentCamera

            zoomFactor = math.pow (1.02,(0.5*(self._LastY - y)))
            self._CurrentZoom = self._CurrentZoom * zoomFactor

            if camera.GetParallelProjection ():
                parallelScale = camera.GetParallelScale ()/zoomFactor
                camera.SetParallelScale (parallelScale)
            else:
                camera.Dolly (zoomFactor)
                renderer.ResetCameraClippingRange ()

            self._LastX = x
            self._LastY = y

            self.Render ()

    def Reset (self):
        if self._CurrentRenderer:
            self._CurrentRenderer.ResetCamera ()
            
        self.Render ()

    def Wireframe (self):
        actors = self._CurrentRenderer.GetActors ()
        numActors = actors.GetNumberOfItems ()
        actors.InitTraversal ()
        for i in range (0,numActors):
            actor = actors.GetNextItem ()
            actor.GetProperty ().SetRepresentationToWireframe ()

        self.Render ()
        
    def Surface (self):
        actors = self._CurrentRenderer.GetActors ()
        numActors = actors.GetNumberOfItems ()
        actors.InitTraversal ()
        for i in range (0,numActors):
            actor = actors.GetNextItem ()
            actor.GetProperty ().SetRepresentationToSurface ()

        self.Render ()

    def PickActor (self,x,y):
        if self._CurrentRenderer:

            renderer = self._CurrentRenderer
            picker = self._Picker
            
            windowY = self.get_window ().height
            picker.Pick (x,(windowY - y - 1),0.0,renderer)
            assembly = picker.GetAssembly ()

            if (self._PickedAssembly != None and
                self._PrePickedProperty != None):
                self._PickedAssembly.SetProperty (self._PrePickedProperty)
                # release hold of the property
                self._PrePickedProperty.UnRegister (self._PrePickedProperty)
                self._PrePickedProperty = None

            if (assembly != None):
                self._PickedAssembly = assembly
                self._PrePickedProperty = self._PickedAssembly.GetProperty ()
                # hold onto the property
                self._PrePickedProperty.Register (self._PrePickedProperty)
                self._PickedAssembly.SetProperty (self._PickedProperty)

            self.Render ()


def main ():
    # The main window
    window = gtk.GtkWindow (gtk.WINDOW_TOPLEVEL)
    window.set_title ("A GtkVtkRenderWindow Demo!")
    window.connect ("destroy", gtk.mainquit)
    window.connect ("delete_event", gtk.mainquit)
    window.set_border_width (10)

    # A VBox into which widgets are packed.
    vbox = gtk.GtkVBox (spacing=3)
    window.add (vbox)
    vbox.show ()

    # The GtkVtkRenderWindow
    gvtk = GtkVtkRenderWindow ()
    gvtk.set_usize (400, 400)
    vbox.pack_start (gvtk)
    gvtk.show ()

    # The VTK stuff.
    cone = vtkpython.vtkConeSource ()
    cone.SetResolution (80)
    coneMapper = vtkpython.vtkPolyDataMapper ()
    coneMapper.SetInput (cone.GetOutput ())
    coneActor = vtkpython.vtkActor ()
    coneActor.SetMapper (coneMapper)    
    coneActor.GetProperty ().SetColor (0.5, 0.5, 1.0)
    ren = vtkpython.vtkRenderer ()
    gvtk.GetRenderWindow ().AddRenderer (ren)
    ren.AddActor (coneActor)

    # A simple quit button
    quit = gtk.GtkButton ("Quit!")
    quit.connect ("clicked", window.destroy)
    vbox.pack_start (quit)
    quit.show ()

    # show the main window and start event processing.
    window.show ()
    gtk.mainloop ()


if __name__ == "__main__":
    main ()
