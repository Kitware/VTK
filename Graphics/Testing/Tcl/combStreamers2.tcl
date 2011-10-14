package require vtk
package require vtkinteraction
package require vtktesting

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create pipeline
#
vtkMultiBlockPLOT3DReader pl3d
    pl3d SetXYZFileName "$VTK_DATA_ROOT/Data/combxyz.bin"
    pl3d SetQFileName "$VTK_DATA_ROOT/Data/combq.bin"
    pl3d SetScalarFunctionNumber 100
    pl3d SetVectorFunctionNumber 202
    pl3d Update
    set output [[pl3d GetOutput] GetBlock 0]

vtkPlaneSource ps
    ps SetXResolution 4
    ps SetYResolution 4
    ps SetOrigin 2 -2 26
    ps SetPoint1 2  2 26
    ps SetPoint2 2 -2 32
vtkPolyDataMapper psMapper
    psMapper SetInputConnection [ps GetOutputPort]
vtkActor psActor
    psActor SetMapper psMapper
    [psActor GetProperty] SetRepresentationToWireframe

vtkDashedStreamLine streamer
    streamer SetInputData $output
    streamer SetSourceData [ps GetOutput]
    streamer SetMaximumPropagationTime 100
    streamer SetIntegrationStepLength .2
    streamer SetStepLength .001
    streamer SetNumberOfThreads 1
    streamer SetIntegrationDirectionToForward
vtkPolyDataMapper streamMapper
    streamMapper SetInputConnection [streamer GetOutputPort]
    eval streamMapper SetScalarRange [$output GetScalarRange]
vtkActor streamline
    streamline SetMapper streamMapper

vtkStructuredGridOutlineFilter outline
    outline SetInputData $output
vtkPolyDataMapper outlineMapper
    outlineMapper SetInputConnection [outline GetOutputPort]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper

# Add the actors to the renderer, set the background and size
#
ren1 AddActor psActor
ren1 AddActor outlineActor
ren1 AddActor streamline

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

# prevent the tk window from showing up then start the event loop
wm withdraw .


