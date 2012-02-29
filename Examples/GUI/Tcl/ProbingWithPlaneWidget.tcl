package require vtk
package require vtkinteraction
package require vtktesting

# This example demonstrates how to use the vtkPlaneWidget to probe
# a dataset and then generate contours on the probed data.

# Start by loading some data.
#
vtkMultiBlockPLOT3DReader pl3d
    pl3d SetXYZFileName "$VTK_DATA_ROOT/Data/combxyz.bin"
    pl3d SetQFileName "$VTK_DATA_ROOT/Data/combq.bin"
    pl3d SetScalarFunctionNumber 100
    pl3d SetVectorFunctionNumber 202
    pl3d Update

set pl3dOutput [[pl3d GetOutput] GetBlock 0]

# The plane widget is used probe the dataset.
#
vtkPlaneWidget planeWidget
    planeWidget SetInputData $pl3dOutput
    planeWidget NormalToXAxisOn
    planeWidget SetResolution 20
    planeWidget SetRepresentationToOutline
    planeWidget PlaceWidget
vtkPolyData plane
    planeWidget GetPolyData plane

vtkProbeFilter probe
    probe SetInputData plane
    probe SetSourceData $pl3dOutput

vtkPolyDataMapper contourMapper
    contourMapper SetInputConnection [probe GetOutputPort]
    eval contourMapper SetScalarRange [$pl3dOutput GetScalarRange]
vtkActor contourActor
    contourActor SetMapper contourMapper
    contourActor VisibilityOff

# An outline is shown for context.
vtkStructuredGridOutlineFilter outline
    outline SetInputData $pl3dOutput
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

# Associate the line widget with the interactor

planeWidget SetInteractor iren
planeWidget AddObserver EnableEvent BeginInteraction
planeWidget AddObserver StartInteractionEvent BeginInteraction
planeWidget AddObserver InteractionEvent ProbeData

# Add the actors to the renderer, set the background and size
#
ren1 AddActor outlineActor
ren1 AddActor contourActor

ren1 SetBackground 1 1 1
renWin SetSize 300 300
ren1 SetBackground 0.1 0.2 0.4

set cam1 [ren1 GetActiveCamera]
$cam1 SetClippingRange 3.95297 50
$cam1 SetFocalPoint 9.71821 0.458166 29.3999
$cam1 SetPosition 2.7439 -37.3196 38.7167
$cam1 SetViewUp -0.16123 0.264271 0.950876

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
renWin Render

# Prevent the tk window from showing up then start the event loop.
wm withdraw .

# Actually generate contour lines.
proc BeginInteraction {} {
    planeWidget GetPolyData plane
    contourActor VisibilityOn
}

proc ProbeData {} {
    planeWidget GetPolyData plane
}

