package require vtk
package require vtkinteraction
package require vtktesting

# This example demonstrates how to use Tcl/Tk in a simple GUI to control 
# the color of the spikes on the mace.

# Create a sphere source and actor. This forms the head of the mace.
#
vtkSphereSource sphere
vtkPolyDataMapper   sphereMapper
    sphereMapper SetInput [sphere GetOutput]
vtkLODActor sphereActor
    sphereActor SetMapper sphereMapper

# Create the spikes using a cone source. The cones are glyphed onto the
# the mace head using the sphere normals.
#
vtkConeSource cone
vtkGlyph3D glyph
    glyph SetInput [sphere GetOutput]
    glyph SetSource [cone GetOutput]
    glyph SetVectorModeToUseNormal
    glyph SetScaleModeToScaleByVector
    glyph SetScaleFactor 0.25
vtkPolyDataMapper spikeMapper
    spikeMapper SetInput [glyph GetOutput]
vtkLODActor spikeActor
    spikeActor SetMapper spikeMapper

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size.
#
ren1 AddActor sphereActor
ren1 AddActor spikeActor
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 300 300

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

set cam1 [ren1 GetActiveCamera]
$cam1 Zoom 1.4
iren Initialize

# Create Tk user interface. This is a simple frame with some color sliders.
#
frame .f
label .f.l -text "Spike Color"
scale .f.r -from 0 -to 100 -background #f00 \
	-orient horizontal -command SetColor
scale .f.g -from 0 -to 100 -background #0f0 \
	-orient horizontal -command SetColor
scale .f.b -from 0 -to 100 -background #00f \
	-orient horizontal -command SetColor

set color [[spikeActor GetProperty] GetColor]
.f.r set [expr [lindex $color 0] * 100.0]
.f.g set [expr [lindex $color 1] * 100.0]
.f.b set [expr [lindex $color 2] * 100.0]

pack .f.l .f.r .f.g .f.b -side top
pack .f

proc SetColor {value} {
    [spikeActor GetProperty] SetColor [expr [.f.r get]/100.0] \
	    [expr [.f.g get]/100.0] \ [expr [.f.b get]/100.0]
    renWin Render
}

