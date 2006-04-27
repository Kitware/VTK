package require vtk
package require vtkinteraction

# This script uses a vtkTkRenderWidget to create a
# Tk widget that is associated with a vtkRenderWindow

# Create the GUI: a render widget and a quit button
wm withdraw .
toplevel .top
frame .top.f1
vtkTkRenderWidget .top.f1.r1 -width 400 -height 400
button .top.btn -text Quit -command exit
pack .top.f1.r1 -side left -padx 3 -pady 3 -fill both -expand t
pack .top.f1 -fill both -expand t
pack .top.btn -fill x

# Get the render window associated with the widget
set renWin [.top.f1.r1 GetRenderWindow]
vtkRenderer ren1
$renWin AddRenderer ren1

# bind the mouse events
::vtk::bind_tk_render_widget .top.f1.r1

# create a Cone source and actor
vtkConeSource cone
vtkPolyDataMapper coneMapper
  coneMapper SetInputConnection [cone GetOutputPort]
  coneMapper GlobalImmediateModeRenderingOn
vtkLODActor coneActor
  coneActor SetMapper coneMapper

# Add the actor to the renderer and set the background
ren1 AddViewProp coneActor
ren1 SetBackground 0.1 0.2 0.4
[ren1 GetActiveCamera] Zoom 0.6
