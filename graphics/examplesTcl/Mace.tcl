catch {load vtktcl}
# this is a tcl version of the Mace example
# get the interactor ui
source vtkInt.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create a sphere source and actor
#
vtkSphereSource sphere
vtkPolyDataMapper   sphereMapper
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
vtkPolyDataMapper spikeMapper
    spikeMapper SetInput [glyph GetOutput]
vtkLODActor spikeActor
    spikeActor SetMapper spikeMapper

# Add the actors to the renderer, set the background and size
#
ren1 AddActor sphereActor
ren1 AddActor spikeActor
set cam1 [ren1 GetActiveCamera]
$cam1 Zoom 1.4
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 300 300
renWin Render

set sphereProp [sphereActor GetProperty]
set spikeProp [spikeActor GetProperty]

# Loop through some properties
#
for {set i 0} {$i < 360} {incr i 2} {
    ren1 SetBackground 0.6 0.0 [expr (360.0 - $i) / 400.0]
    $cam1 Azimuth 5
    $sphereProp SetColor 0.5 [expr $i / 440.0] [expr (360.0 - $i) / 400.0]
    $spikeProp SetColor [expr (360.0 - $i) / 440.0] 0.5 [expr $i / 440.0]
    renWin Render
}

# Enable user method to pop-up interactor
#
iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize


# prevent the tk window from showing up then start the event loop
wm withdraw .


