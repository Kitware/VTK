#!/usr/local/bin/python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


from vtkpython import *

# This example demonstrates how to draw 3D polydata (in world coordinates) in
# the 2D overlay plane. Useful for selection loops, etc.

# create the visualization pipeline
#
# create a sphere
sphere = vtkSphereSource()
sphere.SetThetaResolution(10)
sphere.SetPhiResolution(20)

# extract a group of triangles and their boundary edges
gf = vtkGeometryFilter()
gf.SetInput(sphere.GetOutput())
gf.CellClippingOn()
gf.SetCellMinimum(10)
gf.SetCellMaximum(17)
edges = vtkFeatureEdges()
edges.SetInput(gf.GetOutput())

# setup the mapper to draw points from world coordinate system
worldCoordinates = vtkCoordinate()
worldCoordinates.SetCoordinateSystemToWorld()
mapLines = vtkPolyDataMapper2D()
mapLines.SetInput(edges.GetOutput())
mapLines.SetTransformCoordinate(worldCoordinates)
linesActor = vtkActor2D()
linesActor.SetMapper(mapLines)
linesActor.GetProperty().SetColor(0,1,0)

# mapper and actor
mapper = vtkPolyDataMapper()
mapper.SetInput(sphere.GetOutput())
sphereActor = vtkActor()
sphereActor.SetMapper(mapper)

# Create graphics stuff
# Create the RenderWindow, Renderer and both Actors
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren.AddActor(sphereActor)
ren.AddActor2D(linesActor)
renWin.SetSize(250,250)

# render the image
#
cam1 = ren.GetActiveCamera()
cam1.SetClippingRange(0.294791,17.3744)
cam1.SetFocalPoint(0,0,0)
cam1.SetPosition(1.60648,0.00718286,-2.47173)
cam1.SetViewUp(-0.634086,0.655485,-0.410213)
cam1.Zoom(1.25)
iren.Initialize()

renWin.Render()

renWin.SetFileName("drawMesh.tcl.ppm")
#renWin.SaveImageAsPPM()


iren.Start()
