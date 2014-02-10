# This example shows how to generate and manipulate texture coordinates.
# A random cloud of points is generated and then triangulated with
# vtkDelaunay3D. Since these points do not have texture coordinates,
# we generate them with vtkTextureMapToCylinder.

package require vtk

# Begin by generating 25 random points in the unit sphere.
#
vtkPointSource sphere
  sphere SetNumberOfPoints 25

# Triangulate the points with vtkDelaunay3D. This generates a convex hull
# of tetrahedron.
#
vtkDelaunay3D del
  del SetInputConnection [sphere GetOutputPort]
  del SetTolerance 0.01

# The triangulation has texture coordinates generated so we can map
# a texture onto it.
#
vtkTextureMapToCylinder tmapper
  tmapper SetInputConnection [del GetOutputPort]
  tmapper PreventSeamOn

# We scale the texture coordinate to get some repeat patterns.
vtkTransformTextureCoords xform
  xform SetInputConnection [tmapper GetOutputPort]
  xform SetScale 4 4 1

# vtkDataSetMapper internally uses a vtkGeometryFilter to extract the
# surface from the triangulation. The output (which is vtkPolyData) is
# then passed to an internal vtkPolyDataMapper which does the
# rendering.
vtkDataSetMapper mapper
  mapper SetInputConnection [xform GetOutputPort]

# A texture is loaded using an image reader. Textures are simply images.
# The texture is eventually associated with an actor.
#
vtkBMPReader bmpReader
  bmpReader SetFileName "$VTK_DATA_ROOT/Data/masonry.bmp"
vtkTexture atext
  atext SetInputConnection [bmpReader GetOutputPort]
  atext InterpolateOn
vtkActor triangulation
  triangulation SetMapper mapper
  triangulation SetTexture atext

# Create the standard rendering stuff.
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

iren Start

