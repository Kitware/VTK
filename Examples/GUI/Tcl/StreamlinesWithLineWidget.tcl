package require vtk
package require vtkinteraction
package require vtktesting

# This example demonstrates how to use the vtkLineWidget to seed
# and manipulate streamlines. Two line widgets are created. One is
# invoked by pressing 'W', the other by pressing 'L'. Both can exist
# together.

# Start by loading some data.
#
vtkPLOT3DReader pl3d
    pl3d SetXYZFileName "$VTK_DATA_ROOT/Data/combxyz.bin"
    pl3d SetQFileName "$VTK_DATA_ROOT/Data/combq.bin"
    pl3d SetScalarFunctionNumber 100
    pl3d SetVectorFunctionNumber 202
    pl3d Update

# The line widget is used seed the streamlines.
#
vtkLineWidget lineWidget
vtkPolyData seeds
lineWidget SetInput [pl3d GetOutput]
lineWidget SetAlignToYAxis
lineWidget PlaceWidget
lineWidget GetPolyData seeds
lineWidget ClampToBoundsOn

vtkRungeKutta4 rk4
vtkStreamLine streamer
    streamer SetInput [pl3d GetOutput]
    streamer SetSource seeds
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
vtkActor streamline
    streamline SetMapper streamMapper
    streamline VisibilityOff

# The second line widget is used seed more streamlines.
#
vtkLineWidget lineWidget2
vtkPolyData seeds2
lineWidget2 SetInput [pl3d GetOutput]
lineWidget2 PlaceWidget
lineWidget2 GetPolyData seeds2
lineWidget2 SetKeyPressActivationValue L

vtkStreamLine streamer2
    streamer2 SetInput [pl3d GetOutput]
    streamer2 SetSource seeds2
    streamer2 SetMaximumPropagationTime 100
    streamer2 SetIntegrationStepLength .2
    streamer2 SetStepLength .001
    streamer2 SetNumberOfThreads 1
    streamer2 SetIntegrationDirectionToForward
    streamer2 VorticityOn
    streamer2 SetIntegrator rk4
vtkRibbonFilter rf2
    rf2 SetInput [streamer2 GetOutput]
    rf2 SetWidth 0.1
    rf2 SetWidthFactor 5
vtkPolyDataMapper streamMapper2
    streamMapper2 SetInput [rf2 GetOutput]
    eval streamMapper2 SetScalarRange [[pl3d GetOutput] GetScalarRange]
vtkActor streamline2
    streamline2 SetMapper streamMapper2
    streamline2 VisibilityOff

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
lineWidget SetInteractor iren
lineWidget AddObserver StartInteractionEvent BeginInteraction
lineWidget AddObserver InteractionEvent GenerateStreamlines

lineWidget2 SetInteractor iren
lineWidget2 AddObserver StartInteractionEvent BeginInteraction2
lineWidget2 AddObserver EndInteractionEvent GenerateStreamlines2

# Add the actors to the renderer, set the background and size
#
ren1 AddActor outlineActor
ren1 AddActor streamline
ren1 AddActor streamline2

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

# Actually generate streamlines.
proc BeginInteraction {} {
    streamline VisibilityOn
}

proc GenerateStreamlines {} {
    lineWidget GetPolyData seeds 
}

proc BeginInteraction2 {} {
    streamline2 VisibilityOn
}

proc GenerateStreamlines2 {} {
    lineWidget2 GetPolyData seeds2
}

