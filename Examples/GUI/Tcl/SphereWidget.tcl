package require vtk
package require vtkinteraction
package require vtktesting

# This example demonstrates how to use the vtkSphereWidget to seed
# streamlines in a dataset.

# Start by loading some data.
#
vtkPLOT3DReader pl3d
    pl3d SetXYZFileName "$VTK_DATA_ROOT/Data/combxyz.bin"
    pl3d SetQFileName "$VTK_DATA_ROOT/Data/combq.bin"
    pl3d SetScalarFunctionNumber 100
    pl3d SetVectorFunctionNumber 202
    pl3d Update

# The sphere widget is used in conjunction with vtkPointSource to
# generate seeds for streamribbons.
#
vtkPointSource pointSource
    pointSource SetNumberOfPoints 20
vtkSphereWidget sphereWidget
    sphereWidget SetInput [pl3d GetOutput]
    sphereWidget SetRepresentationToWireframe
    sphereWidget PlaceWidget
vtkPolyData sphere
    sphereWidget GetPolyData sphere

vtkRungeKutta4 rk4
vtkStreamLine streamer
    streamer SetInput [pl3d GetOutput]
    streamer SetSource [pointSource GetOutput]
    streamer SetMaximumPropagationTime 100
    streamer SetIntegrationStepLength .2
    streamer SetStepLength .001
    streamer SetNumberOfThreads 1
    streamer SetIntegrationDirectionToForward
    streamer VorticityOn
    streamer SetIntegrator rk4
vtkRibbonFilter rf
    rf SetInput [streamer GetOutput]
    rf SetWidth 0.1
    rf SetWidthFactor 5
vtkPolyDataMapper streamMapper
    streamMapper SetInput [rf GetOutput]
    eval streamMapper SetScalarRange [[pl3d GetOutput] GetScalarRange]
vtkActor streamlines
    streamlines SetMapper streamMapper
    streamlines VisibilityOff

# An outline is shown for context.
vtkStructuredGridOutlineFilter outline
    outline SetInput [pl3d GetOutput]
vtkPolyDataMapper outlineMapper
    outlineMapper SetInput [outline GetOutput]
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
sphereWidget SetInteractor iren
sphereWidget AddObserver StartInteractionEvent BeginInteraction
sphereWidget AddObserver InteractionEvent GenerateStreamlines

# Add the actors to the renderer, set the background and size
#
ren1 AddActor outlineActor
ren1 AddActor streamlines

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

# Actually probe the data
proc BeginInteraction {} {
    streamlines VisibilityOn
    renWin Render
}

proc GenerateStreamlines {} {
    pointSource SetRadius [sphereWidget GetRadius]
    eval pointSource SetCenter [sphereWidget GetCenter]
}

