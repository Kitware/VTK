package require vtk
package require vtkinteraction

# Create the RenderWindow, Renderer and both Actors
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# load in the image
#
vtkTIFFReader pnmReader
pnmReader SetFileName "$VTK_DATA_ROOT/Data/beach.tif"

vtkImageActor ia
ia SetInput [pnmReader GetOutput]

# Add the actors to the renderer, set the background and size
ren1 AddActor ia
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 400 400

# render the image
iren AddObserver UserEvent {wm deiconify .vtkInteract}
renWin Render

set cam1 [ren1 GetActiveCamera]
$cam1 Elevation -30
$cam1 Roll -20
ren1 ResetCameraClippingRange
renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .





