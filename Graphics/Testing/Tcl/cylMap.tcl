package require vtk

# Generate texture coordinates on a "random" sphere.

# create some random points in a sphere
#
vtkPointSource sphere
  sphere SetNumberOfPoints 25

# triangulate the points
#
vtkDelaunay3D del1
  del1 SetInputConnection [sphere GetOutputPort]
  del1 SetTolerance 0.01
    
# texture map the sphere (using cylindrical coordinate system)
#
vtkTextureMapToCylinder tmapper
  tmapper SetInputConnection [del1 GetOutputPort]
  tmapper PreventSeamOn

vtkTransformTextureCoords xform
  xform SetInputConnection [tmapper GetOutputPort]
  xform SetScale 4 4 1

vtkDataSetMapper mapper
  mapper SetInputConnection [xform GetOutputPort]

# load in the texture map and assign to actor
#
vtkBMPReader bmpReader
  bmpReader SetFileName "$VTK_DATA_ROOT/Data/masonry.bmp"
vtkTexture atext
  atext SetInputConnection [bmpReader GetOutputPort]
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
renWin SetSize 300 300
renWin Render

# render the image
#
renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .



