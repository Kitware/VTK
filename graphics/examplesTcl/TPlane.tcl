catch {load vtktcl}

# get the interactor ui
source vtkInt.tcl

# Create the RenderWindow, Renderer and both Actors
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create a plane source and actor
vtkPlaneSource plane
vtkPolyDataMapper  planeMapper
planeMapper SetInput [plane GetOutput]
vtkActor planeActor
planeActor SetMapper planeMapper


# load in the texture map
#
vtkTexture atext
vtkPNMReader pnmReader
pnmReader SetFileName "../../../data/masonry.ppm"
atext SetInput [pnmReader GetOutput]
atext InterpolateOn
planeActor SetTexture atext

vtkImageViewer view
view SetInput [pnmReader GetOutput]
view Render;

# Add the actors to the renderer, set the background and size
ren1 AddActor planeActor
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 500 500

# render the image
iren Initialize
iren SetUserMethod {wm deiconify .vtkInteract}
set cam1 [ren1 GetActiveCamera]
$cam1 Elevation -30
$cam1 Roll -20
renWin Render

#renWin SetFileName "TPlane.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .





