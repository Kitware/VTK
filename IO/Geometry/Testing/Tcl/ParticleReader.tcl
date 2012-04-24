package require vtk
package require vtkinteraction


# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin


vtkParticleReader reader
  reader SetFileName "$VTK_DATA_ROOT/Data/Particles.raw"
  reader SetDataByteOrderToBigEndian

vtkPolyDataMapper mapper
    mapper SetInputConnection [reader GetOutputPort]
    mapper SetScalarRange 4 9
    mapper SetPiece 1
    mapper SetNumberOfPieces 2
vtkActor actor
    actor SetMapper mapper
    [actor GetProperty] SetPointSize 2.5

# Add the actors to the renderer, set the background and size
#
ren1 AddActor actor
ren1 SetBackground 0 0 0
renWin SetSize 200 200

# Get handles to some useful objects
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize
renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .

