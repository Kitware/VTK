catch {load vtktcl}
# Test the texture transformation object

# Get the interactor
source vtkInt.tcl

# load in the texture map
#
vtkPNMReader pnmReader
  pnmReader SetFileName "../../../data/masonry.ppm"
vtkTexture atext
  atext SetInput [pnmReader GetOutput]
  atext InterpolateOn

# create a plane source and actor
vtkPlaneSource plane
vtkTransformTextureCoords trans
  trans SetInput [plane GetOutput]
  trans SetScale 2 3 1
  trans FlipSOn
  trans SetPosition 0.5 1.0 0.0;#need to do this because of non-zero origin
vtkDataSetMapper  planeMapper
  planeMapper SetInput [trans GetOutput]
vtkActor planeActor
  planeActor SetMapper planeMapper
  planeActor SetTexture atext

# Create graphics stuff
vtkRenderMaster rm
set renWin [rm MakeRenderWindow]
set ren1   [$renWin MakeRenderer]
set iren   [$renWin MakeRenderWindowInteractor]

# Add the actors to the renderer, set the background and size
$ren1 AddActors planeActor
$ren1 SetBackground 0.1 0.2 0.4
$iren SetUserMethod {wm deiconify .vtkInteract}
$renWin SetSize 500 500
$renWin Render

#$renWin SetFileName "texTrans.tcl.ppm"
#$renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .





