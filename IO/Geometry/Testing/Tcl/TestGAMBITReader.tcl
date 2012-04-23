package require vtk
package require vtkinteraction

# Read some Fluent GAMBIT in ASCII form
vtkGAMBITReader reader
   reader SetFileName "$VTK_DATA_ROOT/Data/prism.neu"
vtkDataSetMapper mapper
   mapper SetInputConnection [reader GetOutputPort]
vtkActor actor
   actor SetMapper mapper

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor actor

renWin SetSize 300 300
iren Initialize
renWin Render

iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .
