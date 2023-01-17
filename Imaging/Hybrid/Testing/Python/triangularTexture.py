#!/usr/bin/env python
from vtkmodules.vtkCommonCore import (
    vtkFloatArray,
    vtkPoints,
)
from vtkmodules.vtkCommonDataModel import (
    vtkCellArray,
    vtkPointData,
    vtkPolyData,
)
from vtkmodules.vtkImagingHybrid import vtkTriangularTexture
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
    vtkTexture,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

#
# create a triangular texture
#
# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
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
tCoords = vtkFloatArray()
tCoords.SetNumberOfComponents(2)
tCoords.InsertTuple2(0,0.0,0.0)
tCoords.InsertTuple2(1,1.0,0.0)
tCoords.InsertTuple2(2,.5,.86602540378443864676)
tCoords.InsertTuple2(3,0.0,0.0)
tCoords.InsertTuple2(4,1.0,0.0)
tCoords.InsertTuple2(5,.5,.86602540378443864676)
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
triangleMapper.SetInputData(triangle)
aTexture = vtkTexture()
aTexture.SetInputConnection(aTriangularTexture.GetOutputPort())
triangleActor = vtkActor()
triangleActor.SetMapper(triangleMapper)
triangleActor.SetTexture(aTexture)
ren1.SetBackground(.3,.7,.2)
ren1.AddActor(triangleActor)
ren1.ResetCamera()
ren1.GetActiveCamera().Zoom(1.5)
# render the image
#
iren.Initialize()
# prevent the tk window from showing up then start the event loop
# --- end of script --
