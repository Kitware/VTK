catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }


# include get the vtk interactor ui
source $VTK_TCL/vtkInt.tcl
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

vtkCleanPolyData clean
  clean SetInput [toGeom GetOutput]

vtkTextureMapToCylinder tmapper
  tmapper SetInput [clean GetOutput]
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
  bmpReader SetFileName "$VTK_DATA/masonry.bmp"
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



