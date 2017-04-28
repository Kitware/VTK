#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# read the football dataset:
#
reader = vtk.vtkUnstructuredGridReader()
reader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/PentaHexa.vtk")
reader.Update()
# Clip
#
plane = vtk.vtkPlane()
plane.SetNormal(1,1,0)
clip = vtk.vtkClipDataSet()
clip.SetInputConnection(reader.GetOutputPort())
clip.SetClipFunction(plane)
clip.GenerateClipScalarsOn()
g = vtk.vtkDataSetSurfaceFilter()
g.SetInputConnection(clip.GetOutputPort())
map = vtk.vtkPolyDataMapper()
map.SetInputConnection(g.GetOutputPort())
clipActor = vtk.vtkActor()
clipActor.SetMapper(map)
# Contour
#
contour = vtk.vtkContourFilter()
contour.SetInputConnection(reader.GetOutputPort())
contour.SetValue(0,0.125)
contour.SetValue(1,0.25)
contour.SetValue(2,0.5)
contour.SetValue(3,0.75)
contour.SetValue(4,1.0)
g2 = vtk.vtkDataSetSurfaceFilter()
g2.SetInputConnection(contour.GetOutputPort())
map2 = vtk.vtkPolyDataMapper()
map2.SetInputConnection(g2.GetOutputPort())
map2.ScalarVisibilityOff()
contourActor = vtk.vtkActor()
contourActor.SetMapper(map2)
contourActor.GetProperty().SetColor(1,0,0)
contourActor.GetProperty().SetRepresentationToWireframe()
# Triangulate
tris = vtk.vtkDataSetTriangleFilter()
tris.SetInputConnection(reader.GetOutputPort())
shrink = vtk.vtkShrinkFilter()
shrink.SetInputConnection(tris.GetOutputPort())
shrink.SetShrinkFactor(.8)
map3 = vtk.vtkDataSetMapper()
map3.SetInputConnection(shrink.GetOutputPort())
map3.SetScalarRange(0,26)
triActor = vtk.vtkActor()
triActor.SetMapper(map3)
triActor.AddPosition(2,0,0)
# Create graphics stuff
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(clipActor)
ren1.AddActor(contourActor)
ren1.AddActor(triActor)
ren1.SetBackground(1,1,1)
renWin.Render()
# render the image
#
iren.Initialize()
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
