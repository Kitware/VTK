package require vtk
package require vtkinteraction
package require vtktesting

# create pipeline
#
vtkPLOT3DReader pl3d
    pl3d SetXYZFileName "$VTK_DATA_ROOT/Data/combxyz.bin"
    pl3d SetQFileName "$VTK_DATA_ROOT/Data/combq.bin"
    pl3d SetScalarFunctionNumber 100
    pl3d SetVectorFunctionNumber 202
    pl3d Update

# create a crazy implicit function
set center [[pl3d GetOutput] GetCenter]
vtkSphere sphere
    eval sphere SetCenter $center
    sphere SetRadius 2.0
vtkSphere sphere2
    sphere2 SetCenter [expr [lindex $center 0] + 4.0] [lindex $center 1] \
            [lindex $center 2]
    sphere2 SetRadius 4.0
vtkImplicitBoolean bool
    bool SetOperationTypeToUnion
    bool AddFunction sphere
    bool AddFunction sphere2

# clip the structured grid to produce a tetrahedral mesh
vtkClipDataSet clip
    clip SetInputConnection [pl3d GetOutputPort]
    clip SetClipFunction bool
    clip InsideOutOn

vtkGeometryFilter gf
    gf SetInputConnection [clip GetOutputPort]
vtkPolyDataMapper clipMapper
    clipMapper SetInputConnection [gf GetOutputPort]
vtkActor clipActor
    clipActor SetMapper clipMapper

vtkStructuredGridOutlineFilter outline
    outline SetInputConnection [pl3d GetOutputPort]
vtkPolyDataMapper outlineMapper
    outlineMapper SetInputConnection [outline GetOutputPort]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor clipActor
ren1 AddActor outlineActor

ren1 SetBackground 1 1 1
renWin SetSize 250 250
ren1 SetBackground 0.1 0.2 0.4

set cam1 [ren1 GetActiveCamera]
$cam1 SetClippingRange 3.95297 50
$cam1 SetFocalPoint 8.88908 0.595038 29.3342
$cam1 SetPosition -12.3332 31.7479 41.2387
$cam1 SetViewUp 0.060772 -0.319905 0.945498
iren Initialize

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .
