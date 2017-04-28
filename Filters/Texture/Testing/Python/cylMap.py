#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Generate texture coordinates on a "random" sphere.
# create some random points in a sphere
#
sphere = vtk.vtkPointSource()
sphere.SetNumberOfPoints(25)
# triangulate the points
#
del1 = vtk.vtkDelaunay3D()
del1.SetInputConnection(sphere.GetOutputPort())
del1.SetTolerance(0.01)
# texture map the sphere (using cylindrical coordinate system)
#
tmapper = vtk.vtkTextureMapToCylinder()
tmapper.SetInputConnection(del1.GetOutputPort())
tmapper.PreventSeamOn()
xform = vtk.vtkTransformTextureCoords()
xform.SetInputConnection(tmapper.GetOutputPort())
xform.SetScale(4,4,1)
mapper = vtk.vtkDataSetMapper()
mapper.SetInputConnection(xform.GetOutputPort())
# load in the texture map and assign to actor
#
bmpReader = vtk.vtkBMPReader()
bmpReader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/masonry.bmp")
atext = vtk.vtkTexture()
atext.SetInputConnection(bmpReader.GetOutputPort())
atext.InterpolateOn()
triangulation = vtk.vtkActor()
triangulation.SetMapper(mapper)
triangulation.SetTexture(atext)
# Create rendering stuff
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(triangulation)
ren1.SetBackground(1,1,1)
renWin.SetSize(300,300)
renWin.Render()
# render the image
#
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
