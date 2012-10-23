#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
bmpReader = vtk.vtkBMPReader()
bmpReader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/masonry.bmp")
texture = vtk.vtkTexture()
texture.SetInputConnection(bmpReader.GetOutputPort())
triangleStripPoints = vtk.vtkPoints()
triangleStripPoints.SetNumberOfPoints(5)
triangleStripPoints.InsertPoint(0,0,1,0)
triangleStripPoints.InsertPoint(1,0,0,.5)
triangleStripPoints.InsertPoint(2,1,1,.3)
triangleStripPoints.InsertPoint(3,1,0,.6)
triangleStripPoints.InsertPoint(4,2,1,.1)
triangleStripTCoords = vtk.vtkFloatArray()
triangleStripTCoords.SetNumberOfComponents(2)
triangleStripTCoords.SetNumberOfTuples(5)
triangleStripTCoords.InsertTuple2(0,0,1)
triangleStripTCoords.InsertTuple2(1,0,0)
triangleStripTCoords.InsertTuple2(2,.5,1)
triangleStripTCoords.InsertTuple2(3,.5,0)
triangleStripTCoords.InsertTuple2(4,1,1)
triangleStripPointScalars = vtk.vtkFloatArray()
triangleStripPointScalars.SetNumberOfTuples(5)
triangleStripPointScalars.InsertValue(0,1)
triangleStripPointScalars.InsertValue(1,0)
triangleStripPointScalars.InsertValue(2,0)
triangleStripPointScalars.InsertValue(3,0)
triangleStripPointScalars.InsertValue(4,0)
triangleStripCellScalars = vtk.vtkFloatArray()
triangleStripCellScalars.SetNumberOfTuples(1)
triangleStripCellScalars.InsertValue(0,1)
triangleStripPointNormals = vtk.vtkFloatArray()
triangleStripPointNormals.SetNumberOfComponents(3)
triangleStripPointNormals.SetNumberOfTuples(5)
triangleStripPointNormals.InsertTuple3(0,0,0,1)
triangleStripPointNormals.InsertTuple3(1,0,1,0)
triangleStripPointNormals.InsertTuple3(2,0,1,1)
triangleStripPointNormals.InsertTuple3(3,1,0,0)
triangleStripPointNormals.InsertTuple3(4,1,0,1)
triangleStripCellNormals = vtk.vtkFloatArray()
triangleStripCellNormals.SetNumberOfComponents(3)
triangleStripCellNormals.SetNumberOfTuples(1)
triangleStripCellNormals.InsertTuple3(0,0,0,1)
aTriangleStrip = vtk.vtkTriangleStrip()
aTriangleStrip.GetPointIds().SetNumberOfIds(5)
aTriangleStrip.GetPointIds().SetId(0,0)
aTriangleStrip.GetPointIds().SetId(1,1)
aTriangleStrip.GetPointIds().SetId(2,2)
aTriangleStrip.GetPointIds().SetId(3,3)
aTriangleStrip.GetPointIds().SetId(4,4)
lut = vtk.vtkLookupTable()
lut.SetNumberOfColors(5)
lut.SetTableValue(0,0,0,1,1)
lut.SetTableValue(1,0,1,0,1)
lut.SetTableValue(2,0,1,1,1)
lut.SetTableValue(3,1,0,0,1)
lut.SetTableValue(4,1,0,1,1)
masks = "0 1 2 3 4 5 6 7 10 11 14 15 16 18 20 22 26 30"
i = 0
j = 0
k = 0
types = "strip triangle"
for type in types.split():
    for mask in masks.split():
        locals()[get_variable_name("grid", i, "")] = vtk.vtkUnstructuredGrid()
        locals()[get_variable_name("grid", i, "")].Allocate(1,1)
        locals()[get_variable_name("grid", i, "")].InsertNextCell(aTriangleStrip.GetCellType(),aTriangleStrip.GetPointIds())
        locals()[get_variable_name("grid", i, "")].SetPoints(triangleStripPoints)
        locals()[get_variable_name("geometry", i, "")] = vtk.vtkGeometryFilter()
        locals()[get_variable_name("geometry", i, "")].SetInputData(locals()[get_variable_name("grid", i, "")])
        locals()[get_variable_name("triangles", i, "")] = vtk.vtkTriangleFilter()
        locals()[get_variable_name("triangles", i, "")].SetInputConnection(locals()[get_variable_name("geometry", i, "")].GetOutputPort())
        locals()[get_variable_name("mapper", i, "")] = vtk.vtkPolyDataMapper()
        if (type == "strip"):
            locals()[get_variable_name("mapper", i, "")].SetInputConnection(locals()[get_variable_name("geometry", i, "")].GetOutputPort())
            pass
        if (type == "triangle"):
            locals()[get_variable_name("mapper", i, "")].SetInputConnection(locals()[get_variable_name("triangles", i, "")].GetOutputPort())
            pass
        locals()[get_variable_name("mapper", i, "")].SetLookupTable(lut)
        locals()[get_variable_name("mapper", i, "")].SetScalarRange(0,4)
        locals()[get_variable_name("actor", i, "")] = vtk.vtkActor()
        locals()[get_variable_name("actor", i, "")].SetMapper(locals()[get_variable_name("mapper", i, "")])
        if (expr.expr(globals(), locals(),["mask","&","1"]) != 0):
            locals()[get_variable_name("grid", i, "")].GetPointData().SetNormals(triangleStripPointNormals)
            pass
        if (expr.expr(globals(), locals(),["mask","&","2"]) != 0):
            locals()[get_variable_name("grid", i, "")].GetPointData().SetScalars(triangleStripPointScalars)
            locals()[get_variable_name("mapper", i, "")].SetScalarModeToUsePointData()
            pass
        if (expr.expr(globals(), locals(),["mask","&","4"]) != 0):
            locals()[get_variable_name("grid", i, "")].GetPointData().SetTCoords(triangleStripTCoords)
            locals()[get_variable_name("actor", i, "")].SetTexture(texture)
            pass
        if (expr.expr(globals(), locals(),["mask","&","8"]) != 0):
            locals()[get_variable_name("grid", i, "")].GetCellData().SetScalars(triangleStripCellScalars)
            locals()[get_variable_name("mapper", i, "")].SetScalarModeToUseCellData()
            pass
        if (expr.expr(globals(), locals(),["mask","&","16"]) != 0):
            locals()[get_variable_name("grid", i, "")].GetCellData().SetNormals(triangleStripCellNormals)
            pass
        locals()[get_variable_name("actor", i, "")].AddPosition(expr.expr(globals(), locals(),["j","*","2"]),expr.expr(globals(), locals(),["k","*","2"]),0)
        ren1.AddActor(locals()[get_variable_name("actor", i, "")])
        j = j + 1
        if (j >= 6):
            j = 0
            k = k + 1
            pass
        i = i + 1

        pass

    pass
renWin.SetSize(480,480)
ren1.SetBackground(.7,.3,.1)
ren1.ResetCameraClippingRange()
renWin.Render()
# render the image
#
iren.Initialize()
# --- end of script --
