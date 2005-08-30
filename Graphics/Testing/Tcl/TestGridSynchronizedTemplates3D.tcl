package require vtk
package require vtkinteraction

# cut data
vtkPLOT3DReader pl3d
    pl3d SetXYZFileName "$VTK_DATA_ROOT/Data/combxyz.bin"
    pl3d SetQFileName "$VTK_DATA_ROOT/Data/combq.bin"
    pl3d SetScalarFunctionNumber 100
    pl3d SetVectorFunctionNumber 202
    pl3d Update
set range [[[[pl3d GetOutput] GetPointData] GetScalars] GetRange]
set min [lindex $range 0]
set max [lindex $range 1]
set value [expr ($min + $max) / 2.0]

#vtkGridSynchronizedTemplates3D cf
vtkContourFilter cf
    cf SetInputConnection [pl3d GetOutputPort]
    cf SetValue 0 $value
	#cf ComputeNormalsOff

vtkPolyDataMapper cfMapper
	cfMapper ImmediateModeRenderingOn
    cfMapper SetInputConnection [cf GetOutputPort]
    eval cfMapper SetScalarRange \
      [[[[pl3d GetOutput] GetPointData] GetScalars] GetRange]
vtkActor cfActor
    cfActor SetMapper cfMapper

#outline
vtkStructuredGridOutlineFilter outline
    outline SetInputConnection [pl3d GetOutputPort]
vtkPolyDataMapper outlineMapper
    outlineMapper SetInputConnection [outline GetOutputPort]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper
    eval [outlineActor GetProperty] SetColor 0 0 0

## Graphics stuff
# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor outlineActor
ren1 AddActor cfActor
ren1 SetBackground 1 1 1
renWin SetSize 400 400

set cam1 [ren1 GetActiveCamera]
$cam1 SetClippingRange 3.95297 50
$cam1 SetFocalPoint 9.71821 0.458166 29.3999
$cam1 SetPosition 2.7439 -37.3196 38.7167
$cam1 SetViewUp -0.16123 0.264271 0.950876
iren Initialize

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

# loop over surfaces
for {set i 0} {$i < 17} {incr i} {
   cf SetValue 0 [expr $min + ($i/16.0)*($max - $min)]
   renWin Render
}

cf SetValue 0 [expr $min + (0.2)*($max - $min)]
renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .

