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
vtkStructuredPointsReader reader
    reader SetFileName "$VTK_DATA_ROOT/Data/ironProt.vtk"
vtkRecursiveDividingCubes iso
    iso SetInput [reader GetOutput]
    iso SetValue 128
    iso SetDistance .5
    iso SetIncrement 2
vtkPolyDataMapper isoMapper
    isoMapper SetInput [iso GetOutput]
    isoMapper ScalarVisibilityOff
    isoMapper ImmediateModeRenderingOn
vtkActor isoActor
    isoActor SetMapper isoMapper
    eval [isoActor GetProperty] SetColor $bisque

vtkOutlineFilter outline
    outline SetInput [reader GetOutput]
vtkPolyDataMapper outlineMapper
    outlineMapper SetInput [outline GetOutput]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper
    eval [outlineActor GetProperty] SetColor $black

# Add the actors to the renderer, set the background and size
#
ren1 AddActor outlineActor
ren1 AddActor isoActor
renWin SetSize 250 250
ren1 SetBackground 0.1 0.2 0.4

vtkCamera cam1
    cam1 SetClippingRange 19.1589 957.946
    cam1 SetFocalPoint 33.7014 26.706 30.5867
    cam1 SetPosition 150.841 89.374 -107.462
    cam1 SetViewUp -0.190015 0.944614 0.267578
    cam1 Dolly 3
ren1 SetActiveCamera cam1

iren Initialize

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .
