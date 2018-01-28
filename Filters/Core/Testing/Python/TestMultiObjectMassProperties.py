#!/usr/bin/env python

# Demonstrate computation of volumes and areas of objects
# defined by polygonal meshes
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create the RenderWindow, Renderer
#
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer( ren )

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Create "blocks" out of combinations of polygons, with
# different orientations, each with total volume of 1
# and total area of 6. Additionally, a non-manifold
# polygonal "mess" tests the invalid state.
polyData = vtk.vtkPolyData()
pts = vtk.vtkPoints()
polys = vtk.vtkCellArray()
polyData.SetPoints(pts)
polyData.SetPolys(polys)

pts.SetNumberOfPoints(42)
# First block
pts.SetPoint(0, -2, -2, -0.5)
pts.SetPoint(1, -1, -2, -0.5)
pts.SetPoint(2, -2, -1, -0.5)
pts.SetPoint(3, -1, -1, -0.5)
pts.SetPoint(4, -2, -2, 0.5)
pts.SetPoint(5, -1, -2, 0.5)
pts.SetPoint(6, -2, -1, 0.5)
pts.SetPoint(7, -1, -1, 0.5)

# Second block
pts.SetPoint(8, 1, -2, -0.5)
pts.SetPoint(9, 2, -2, -0.5)
pts.SetPoint(10, 1, -1, -0.5)
pts.SetPoint(11, 2, -1, -0.5)
pts.SetPoint(12, 1, -2, 0.5)
pts.SetPoint(13, 2, -2, 0.5)
pts.SetPoint(14, 1, -1, 0.5)
pts.SetPoint(15, 2, -1, 0.5)

# Third block
pts.SetPoint(16, -2, 1, -0.5)
pts.SetPoint(17, -1, 1, -0.5)
pts.SetPoint(18, -2, 2, -0.5)
pts.SetPoint(19, -1, 2, -0.5)
pts.SetPoint(20, -2, 1, 0.5)
pts.SetPoint(21, -1, 1, 0.5)
pts.SetPoint(22, -2, 2, 0.5)
pts.SetPoint(23, -1, 2, 0.5)

# Fourth block
pts.SetPoint(24, 1, 1, -0.5)
pts.SetPoint(25, 2, 1, -0.5)
pts.SetPoint(26, 1, 2, -0.5)
pts.SetPoint(27, 2, 2, -0.5)
pts.SetPoint(28, 1, 1, 0.5)
pts.SetPoint(29, 2, 1, 0.5)
pts.SetPoint(30, 1, 2, 0.5)
pts.SetPoint(31, 2, 2, 0.5)

# Invalid poly mess (a "X")
pts.SetPoint(32, 0, -0.5, -0.5)
pts.SetPoint(33, 0,  0.5, -0.5)
pts.SetPoint(34, -0.5, -0.5, 0)
pts.SetPoint(35, 0, -0.5, 0)
pts.SetPoint(36, 0.5, -0.5, 0)
pts.SetPoint(37, -0.5, 0.5, 0)
pts.SetPoint(38, 0, 0.5, 0)
pts.SetPoint(39, 0.5, 0.5, 0)
pts.SetPoint(40, 0, -0.5, 0.5)
pts.SetPoint(41, 0, 0.5, 0.5)

# First block - all quads, consistent order
polys.InsertNextCell(4)
polys.InsertCellPoint(0)
polys.InsertCellPoint(1)
polys.InsertCellPoint(3)
polys.InsertCellPoint(2)

polys.InsertNextCell(4)
polys.InsertCellPoint(4)
polys.InsertCellPoint(6)
polys.InsertCellPoint(7)
polys.InsertCellPoint(5)

polys.InsertNextCell(4)
polys.InsertCellPoint(0)
polys.InsertCellPoint(4)
polys.InsertCellPoint(5)
polys.InsertCellPoint(1)

polys.InsertNextCell(4)
polys.InsertCellPoint(3)
polys.InsertCellPoint(7)
polys.InsertCellPoint(6)
polys.InsertCellPoint(2)

polys.InsertNextCell(4)
polys.InsertCellPoint(1)
polys.InsertCellPoint(5)
polys.InsertCellPoint(7)
polys.InsertCellPoint(3)

polys.InsertNextCell(4)
polys.InsertCellPoint(2)
polys.InsertCellPoint(6)
polys.InsertCellPoint(4)
polys.InsertCellPoint(0)

# Second block - quads with a reversed polygon
polys.InsertNextCell(4)
polys.InsertCellPoint(8)
polys.InsertCellPoint(9)
polys.InsertCellPoint(11)
polys.InsertCellPoint(10)

polys.InsertNextCell(4)
polys.InsertCellPoint(12)
polys.InsertCellPoint(13)
polys.InsertCellPoint(15)
polys.InsertCellPoint(14)

polys.InsertNextCell(4)
polys.InsertCellPoint(8)
polys.InsertCellPoint(12)
polys.InsertCellPoint(13)
polys.InsertCellPoint(9)

polys.InsertNextCell(4)
polys.InsertCellPoint(11)
polys.InsertCellPoint(15)
polys.InsertCellPoint(14)
polys.InsertCellPoint(10)

polys.InsertNextCell(4)
polys.InsertCellPoint(9)
polys.InsertCellPoint(13)
polys.InsertCellPoint(15)
polys.InsertCellPoint(11)

polys.InsertNextCell(4)
polys.InsertCellPoint(10)
polys.InsertCellPoint(14)
polys.InsertCellPoint(12)
polys.InsertCellPoint(8)

# Third block - with triangles + quads
polys.InsertNextCell(3)
polys.InsertCellPoint(16)
polys.InsertCellPoint(17)
polys.InsertCellPoint(19)

polys.InsertNextCell(3)
polys.InsertCellPoint(19)
polys.InsertCellPoint(18)
polys.InsertCellPoint(16)

polys.InsertNextCell(4)
polys.InsertCellPoint(20)
polys.InsertCellPoint(22)
polys.InsertCellPoint(23)
polys.InsertCellPoint(21)

polys.InsertNextCell(4)
polys.InsertCellPoint(16)
polys.InsertCellPoint(20)
polys.InsertCellPoint(21)
polys.InsertCellPoint(17)

polys.InsertNextCell(4)
polys.InsertCellPoint(19)
polys.InsertCellPoint(23)
polys.InsertCellPoint(22)
polys.InsertCellPoint(18)

polys.InsertNextCell(4)
polys.InsertCellPoint(17)
polys.InsertCellPoint(21)
polys.InsertCellPoint(23)
polys.InsertCellPoint(19)

polys.InsertNextCell(4)
polys.InsertCellPoint(18)
polys.InsertCellPoint(22)
polys.InsertCellPoint(20)
polys.InsertCellPoint(16)

# Fourth block - with triangles + quads + reversals
polys.InsertNextCell(3)
polys.InsertCellPoint(24)
polys.InsertCellPoint(25)
polys.InsertCellPoint(27)

polys.InsertNextCell(3)
polys.InsertCellPoint(26)
polys.InsertCellPoint(27)
polys.InsertCellPoint(24)

polys.InsertNextCell(4)
polys.InsertCellPoint(28)
polys.InsertCellPoint(30)
polys.InsertCellPoint(31)
polys.InsertCellPoint(29)

polys.InsertNextCell(4)
polys.InsertCellPoint(24)
polys.InsertCellPoint(28)
polys.InsertCellPoint(29)
polys.InsertCellPoint(25)

polys.InsertNextCell(4)
polys.InsertCellPoint(27)
polys.InsertCellPoint(31)
polys.InsertCellPoint(30)
polys.InsertCellPoint(26)

polys.InsertNextCell(4)
polys.InsertCellPoint(25)
polys.InsertCellPoint(29)
polys.InsertCellPoint(31)
polys.InsertCellPoint(27)

polys.InsertNextCell(4)
polys.InsertCellPoint(26)
polys.InsertCellPoint(30)
polys.InsertCellPoint(28)
polys.InsertCellPoint(24)

# Non-manifold Poly mess - four quads forming a "X"
polys.InsertNextCell(4)
polys.InsertCellPoint(32)
polys.InsertCellPoint(33)
polys.InsertCellPoint(38)
polys.InsertCellPoint(35)

polys.InsertNextCell(4)
polys.InsertCellPoint(34)
polys.InsertCellPoint(35)
polys.InsertCellPoint(38)
polys.InsertCellPoint(37)

polys.InsertNextCell(4)
polys.InsertCellPoint(35)
polys.InsertCellPoint(36)
polys.InsertCellPoint(39)
polys.InsertCellPoint(38)

polys.InsertNextCell(4)
polys.InsertCellPoint(35)
polys.InsertCellPoint(38)
polys.InsertCellPoint(41)
polys.InsertCellPoint(40)

# Compute volumes
massP = vtk.vtkMultiObjectMassProperties()
massP.SetInputData(polyData)
massP.Update()

print("Mass Propertites: {0}".format(massP))
print("Number Objects: {0}".format(massP.GetNumberOfObjects()))
print("All Valid: {0}".format(massP.GetAllValid()))
print("Total Area: {0}".format(massP.GetTotalArea()))
print("Total Volume: {0}".format(massP.GetTotalVolume()))

numObjects = massP.GetNumberOfObjects()
validArray = massP.GetOutput().GetFieldData().GetArray("ObjectValidity")
areaArray = massP.GetOutput().GetFieldData().GetArray("ObjectAreas")
volArray = massP.GetOutput().GetFieldData().GetArray("ObjectVolumes")
print("Object ID, Valid, Area, Volume")
for i in range(0,numObjects) :
    valid = validArray.GetTuple1(i)
    area = areaArray.GetTuple1(i);
    vol = volArray.GetTuple1(i)
    print(i, valid, area, vol)

mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(massP.GetOutputPort())
mapper.ScalarVisibilityOff()

actor = vtk.vtkActor()
actor.SetMapper(mapper)

# Render it
ren.AddActor(actor)

ren.GetActiveCamera().SetPosition( 0.5, 0.5, 1 )
ren.GetActiveCamera().SetFocalPoint( 0, 0, 0 )
ren.ResetCamera()

renWin.Render()
iren.Start()
