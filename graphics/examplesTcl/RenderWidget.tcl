catch {load vtktcl}

# This script uses a vtkTkRenderWidget to create a
# Tk widget that is associated with a vtkRenderWindow.
source TkInteractor.tcl

# Load in standard bindings for interactor

# Create the GUI: two renderer widgets and a quit button
#
wm withdraw .
toplevel .top -visual {truecolor 24}
frame .top.f1

vtkTkRenderWidget .top.f1.r1 -width 300 -height 300 
    BindTkRenderWidget .top.f1.r1

vtkTkRenderWidget .top.f1.r2 -width 300 -height 300 
    BindTkRenderWidget .top.f1.r2

button .top.btn  -text Quit -command exit

pack .top.f1.r1 -side left -padx 3 -pady 3 -fill both -expand t
pack .top.f1.r2 -side left -padx 3 -pady 3 -fill both -expand t
pack .top.f1  -fill both -expand t
pack .top.btn -fill x

puts [bind .top.f1.r1]
puts [bind .top.f1.r2]

# Create graphics pipeline
#

# Get the render window associated with the widget.
set renWin1 [.top.f1.r1 GetRenderWindow]
set ren1   [$renWin1 MakeRenderer]

set renWin2 [.top.f1.r2 GetRenderWindow]
set ren2   [$renWin2 MakeRenderer]


# create a sphere source and actor
#
vtkSphereSource sphere
vtkPolyMapper   sphereMapper
    sphereMapper SetInput [sphere GetOutput]
vtkLODActor sphereActor
    sphereActor SetMapper sphereMapper

# create the spikes using a cone source and the sphere source
#
vtkConeSource cone
vtkGlyph3D glyph
    glyph SetInput [sphere GetOutput]
    glyph SetSource [cone GetOutput]
    glyph SetVectorModeToUseNormal
    glyph SetScaleModeToScaleByVector
    glyph SetScaleFactor 0.25
vtkPolyMapper spikeMapper
    spikeMapper SetInput [glyph GetOutput]
vtkLODActor spikeActor
    spikeActor SetMapper spikeMapper

# Add the actors to the renderer, set the background and size
#
$ren1 AddActors sphereActor
$ren1 AddActors spikeActor
$ren1 SetBackground 0.1 0.2 0.4
$renWin1 SetSize 300 300

$ren2 AddActors sphereActor
$ren2 AddActors spikeActor
$ren2 SetBackground 0.1 0.2 0.4
$renWin2 SetSize 300 300
