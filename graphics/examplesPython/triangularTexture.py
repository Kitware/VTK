#!/usr/local/bin/python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

#
# create a triangular texture and save it as a ppm
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
renWin.SetSize(400,400)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)


aTriangularTexture = vtkTriangularTexture()
aTriangularTexture.SetTexturePattern(1)
aTriangularTexture.SetXSize(32)
aTriangularTexture.SetYSize(32)
  

points = vtkPoints()
points.InsertPoint(0,0.0,0.0,0.0)
points.InsertPoint(1,1.0,0.0,0.0)
points.InsertPoint(2,.5,1.0,0.0)
points.InsertPoint(3,1.0,0.0,0.0)
points.InsertPoint(4,0.0,0.0,0.0)
points.InsertPoint(5,.5,-1.0,.5)

tCoords = vtkTCoords()
tCoords.InsertTCoord(0,0.0,0.0,0.0)
tCoords.InsertTCoord(1,1.0,0.0,0.0)
tCoords.InsertTCoord(2,.5,.86602540378443864676,0.0)
tCoords.InsertTCoord(3,0.0,0.0,0.0)
tCoords.InsertTCoord(4,1.0,0.0,0.0)
tCoords.InsertTCoord(5,.5,.86602540378443864676,0.0)

pointData = vtkPointData()
pointData.SetTCoords(tCoords)

triangles = vtkCellArray()
triangles.InsertNextCell(3)
triangles.InsertCellPoint(0)
triangles.InsertCellPoint(1)
triangles.InsertCellPoint(2)
triangles.InsertNextCell(3)
triangles.InsertCellPoint(3)
triangles.InsertCellPoint(4)
triangles.InsertCellPoint(5)

triangle = vtkPolyData()
triangle.SetPolys(triangles)
triangle.SetPoints(points)
triangle.GetPointData().SetTCoords(tCoords)

triangleMapper = vtkPolyDataMapper()
triangleMapper.SetInput(triangle)

aTexture = vtkTexture()
aTexture.SetInput(aTriangularTexture.GetOutput())

triangleActor = vtkActor()
triangleActor.SetMapper(triangleMapper)
triangleActor.SetTexture(aTexture)

ren.SetBackground(.3,.7,.2)
ren.AddActor(triangleActor)
ren.GetActiveCamera().Zoom(1.5)

# render the image
#
iren.Initialize()
renWin.SetFileName("triangularTexture.ppm")

iren.Start()
