package require vtk
package require vtkinteraction
package require vtktesting

# create pipeline
#
vtkDataSetReader reader
    reader SetFileName "$VTK_DATA_ROOT/Data/RectGrid2.vtk"
    reader Update
vtkWarpVector warper
    warper SetInputConnection [reader GetOutputPort]
    warper SetScaleFactor 0.2
vtkExtractGrid extract
    extract SetInputConnection [warper GetOutputPort]
    extract SetVOI 0 100 0 100 7 15

vtkDataSetMapper mapper
    mapper SetInputConnection [extract GetOutputPort]
    mapper SetScalarRange 0.197813 0.710419
vtkActor actor
    actor SetMapper mapper

# Graphics stuff
# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin SetMultiSamples 0
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor actor

ren1 SetBackground 1 1 1
renWin SetSize 400 400

set cam1 [ren1 GetActiveCamera]
    $cam1 SetClippingRange 3.76213 10.712
    $cam1 SetFocalPoint -0.0842503 -0.136905 0.610234
    $cam1 SetPosition 2.53813 2.2678 -5.22172
    $cam1 SetViewUp -0.241047 0.930635 0.275343

iren Initialize

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .
