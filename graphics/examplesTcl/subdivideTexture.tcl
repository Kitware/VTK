catch {load vtktcl}

# include get the vtk interactor ui
source ../../examplesTcl/vtkInt.tcl
# Generate texture coordinates on a "random" sphere.

# create some random points in a sphere
#
vtkPointSource sphere
  sphere SetNumberOfPoints 25

# triangulate the points
#
vtkDelaunay3D del
  del SetInput [sphere GetOutput]
  del SetTolerance 0.01
    
# texture map the sphere (using cylindrical coordinate system)
#
vtkGeometryFilter toGeom
  toGeom SetInput [del GetOutput]

vtkTextureMapToCylinder tmapper
  tmapper SetInput [toGeom GetOutput]
  tmapper PreventSeamOn

vtkCastToConcrete castToPoly
  castToPoly SetInput [tmapper GetOutput]

vtkButterflySubdivisionFilter butterfly
  butterfly SetInput [castToPoly GetPolyDataOutput]
  butterfly SetNumberOfSubdivisions 4

vtkDataSetMapper mapper
  mapper SetInput [butterfly GetOutput]

# load in the texture map and assign to actor
#
vtkBMPReader bmpReader
  bmpReader SetFileName "../../../vtkdata/masonry.bmp"
vtkTexture atext
  atext SetInput [bmpReader GetOutput]
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
renWin Render

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
renWin Render
renWin SetFileName "subdivideTexture.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .



