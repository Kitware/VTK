package require vtk
package require vtkinteraction
package require vtktesting

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderer ren2
vtkRenderWindow renWin
    renWin PointSmoothingOn
    renWin AddRenderer ren1
    renWin AddRenderer ren2
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create pipeline
#
vtkStructuredPointsReader reader
    reader SetFileName "$VTK_DATA_ROOT/Data/ironProt.vtk"
vtkDividingCubes iso
    iso SetInput [reader GetOutput]
    iso SetValue 128
    iso SetDistance 1
    iso SetIncrement 1
vtkPolyDataMapper isoMapper
    isoMapper SetInput [iso GetOutput]
    isoMapper ScalarVisibilityOff
vtkActor isoActor1
    isoActor1 SetMapper isoMapper
    eval [isoActor1 GetProperty] SetDiffuseColor $banana
    eval [isoActor1 GetProperty] SetDiffuse .7
    eval [isoActor1 GetProperty] SetSpecular .5
    eval [isoActor1 GetProperty] SetSpecularPower 30

vtkActor isoActor2
    isoActor2 SetMapper isoMapper
    eval [isoActor2 GetProperty] SetDiffuseColor $banana
    eval [isoActor2 GetProperty] SetDiffuse .7
    eval [isoActor2 GetProperty] SetSpecular .5
    eval [isoActor2 GetProperty] SetSpecularPower 30
    eval [isoActor2 GetProperty] SetPointSize 5

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
ren1 AddActor isoActor1
ren1 SetBackground 1 1 1

ren2 AddActor outlineActor
ren2 AddActor isoActor2
ren2 SetBackground 1 1 1

renWin SetSize 400 400
ren1 SetBackground 0.1 0.2 0.4
ren2 SetBackground 0.1 0.2 0.4

ren1 SetViewport 0 0 .5 1
ren2 SetViewport .5 0 1 1

vtkCamera cam1
    cam1 SetClippingRange 19.1589 957.946
    cam1 SetFocalPoint 33.7014 26.706 30.5867
    cam1 SetPosition 150.841 89.374 -107.462
    cam1 SetViewUp -0.190015 0.944614 0.267578
    cam1 Dolly 2

vtkLight aLight
eval  aLight SetPosition [cam1 GetPosition]
eval  aLight SetFocalPoint [cam1 GetFocalPoint]

ren1 SetActiveCamera cam1
ren1 AddLight aLight

ren2 SetActiveCamera cam1
ren2 AddLight aLight

iren Initialize

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .


