catch {load vtktcl}
# Generate texture coordinates on a "random" sphere.

# get the interactor ui
source vtkInt.tcl

# create some random points in a sphere
#
vtkPointSource sphere
  sphere SetNumberOfPoints 25

# triangulate the points
#
vtkDelaunay3D del
  del SetInput [sphere GetOutput]
  del SetTolerance 0.01
    
# texture map the sphere
#
vtkTextureMapToSphere tmapper
  tmapper SetInput [del GetOutput]
  tmapper PreventSeamOn

vtkTransformTextureCoords xform
  xform SetInput [tmapper GetOutput]
  xform SetScale 4 4 1

vtkDataSetMapper mapper
  mapper SetInput [xform GetOutput]

# load in the texture map and assign to actor
#
vtkPNMReader pnmReader
  pnmReader SetFileName "../../../data/masonry.ppm"
vtkTexture atext
  atext SetInput [pnmReader GetOutput]
  atext InterpolateOn
vtkActor triangulation
  triangulation SetMapper mapper
  triangulation SetTexture atext

# Create rendering stuff
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor triangulation
ren1 SetBackground 1 1 1
renWin SetSize 500 500

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .



