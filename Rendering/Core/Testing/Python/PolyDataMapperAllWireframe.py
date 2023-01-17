#!/usr/bin/env python
from vtkmodules.vtkCommonCore import (
    vtkFloatArray,
    vtkLookupTable,
    vtkPoints,
)
from vtkmodules.vtkCommonDataModel import (
    vtkTriangleStrip,
    vtkUnstructuredGrid,
)
from vtkmodules.vtkFiltersCore import vtkTriangleFilter
from vtkmodules.vtkFiltersGeometry import vtkGeometryFilter
from vtkmodules.vtkIOImage import vtkBMPReader
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

ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

pnmReader = vtkBMPReader()
pnmReader.SetFileName(VTK_DATA_ROOT + "/Data/masonry.bmp")

texture = vtkTexture()
texture.SetInputConnection(pnmReader.GetOutputPort())

triangleStripPoints = vtkPoints()
triangleStripPoints.SetNumberOfPoints(5)
triangleStripPoints.InsertPoint(0, 0, 1, 0)
triangleStripPoints.InsertPoint(1, 0, 0, .5)
triangleStripPoints.InsertPoint(2, 1, 1, .3)
triangleStripPoints.InsertPoint(3, 1, 0, .6)
triangleStripPoints.InsertPoint(4, 2, 1, .1)

triangleStripTCoords = vtkFloatArray()
triangleStripTCoords.SetNumberOfComponents(2)
triangleStripTCoords.SetNumberOfTuples(5)
triangleStripTCoords.InsertTuple2(0, 0, 1)
triangleStripTCoords.InsertTuple2(1, 0, 0)
triangleStripTCoords.InsertTuple2(2, .5, 1)
triangleStripTCoords.InsertTuple2(3, .5, 0)
triangleStripTCoords.InsertTuple2(4, 1, 1)

triangleStripPointScalars = vtkFloatArray()
triangleStripPointScalars.SetNumberOfTuples(5)
triangleStripPointScalars.InsertValue(0, 1)
triangleStripPointScalars.InsertValue(1, 0)
triangleStripPointScalars.InsertValue(2, 0)
triangleStripPointScalars.InsertValue(3, 0)
triangleStripPointScalars.InsertValue(4, 0)

triangleStripCellScalars = vtkFloatArray()
triangleStripCellScalars.SetNumberOfTuples(1)
triangleStripCellScalars.InsertValue(0, 1)

triangleStripPointNormals = vtkFloatArray()
triangleStripPointNormals.SetNumberOfComponents(3)
triangleStripPointNormals.SetNumberOfTuples(5)
triangleStripPointNormals.InsertTuple3(0, 0, 0, 1)
triangleStripPointNormals.InsertTuple3(1, 0, 1, 0)
triangleStripPointNormals.InsertTuple3(2, 0, 1, 1)
triangleStripPointNormals.InsertTuple3(3, 1, 0, 0)
triangleStripPointNormals.InsertTuple3(4, 1, 0, 1)

triangleStripCellNormals = vtkFloatArray()
triangleStripCellNormals.SetNumberOfComponents(3)
triangleStripCellNormals.SetNumberOfTuples(1)
triangleStripCellNormals.InsertTuple3(0, 1, 1, 1)

aTriangleStrip = vtkTriangleStrip()
aTriangleStrip.GetPointIds().SetNumberOfIds(5)
aTriangleStrip.GetPointIds().SetId(0, 0)
aTriangleStrip.GetPointIds().SetId(1, 1)
aTriangleStrip.GetPointIds().SetId(2, 2)
aTriangleStrip.GetPointIds().SetId(3, 3)
aTriangleStrip.GetPointIds().SetId(4, 4)

lut = vtkLookupTable()
lut.SetNumberOfColors(5)
lut.SetTableValue(0, 0, 0, 1, 1)
lut.SetTableValue(1, 0, 1, 0, 1)
lut.SetTableValue(2, 0, 1, 1, 1)
lut.SetTableValue(3, 1, 0, 0, 1)
lut.SetTableValue(4, 1, 0, 1, 1)

masks = [0, 1, 2, 3, 4, 5, 6, 7, 10, 11, 14, 15, 16, 18, 20, 22, 26, 30]
types = ["strip", "triangle"]
i = 0
j = 0
k = 0
for type in types:
    for mask in masks:
        idx = str(i)
        exec("grid" + idx + " = vtkUnstructuredGrid()")
        eval("grid" + idx).Allocate(1, 1)
        eval("grid" + idx).InsertNextCell(
          aTriangleStrip.GetCellType(), aTriangleStrip.GetPointIds())
        eval("grid" + idx).SetPoints(triangleStripPoints)

        exec("geometry" + idx + " = vtkGeometryFilter()")
        eval("geometry" + idx).SetInputData(eval("grid" + idx))

        exec("triangles" + idx + " = vtkTriangleFilter()")
        eval("triangles" + idx).SetInputConnection(
          eval("geometry" + idx).GetOutputPort())

        exec("mapper" + idx + " = vtkPolyDataMapper()")
        if (type == "strip"):
            eval("mapper" + idx).SetInputConnection(
              eval("geometry" + idx).GetOutputPort())

        if (type == "triangle"):
            eval("mapper" + idx).SetInputConnection(
              eval("triangles" + idx).GetOutputPort())

        eval("mapper" + idx).SetLookupTable(lut)
        eval("mapper" + idx).SetScalarRange(0, 4)

        exec("actor" + idx + " = vtkActor()")
        eval("actor" + idx).SetMapper(eval("mapper" + idx))

        if mask & 1 != 0:
            eval("grid" + idx).GetPointData().SetNormals(
              triangleStripPointNormals)
        if mask & 2 != 0:
            eval("grid" + idx).GetPointData().SetScalars(
              triangleStripPointScalars)
            eval("mapper" + idx).SetScalarModeToUsePointData()
        if mask & 4 != 0:
            eval("grid" + idx).GetPointData().SetTCoords(
              triangleStripTCoords)
            eval("actor" + idx).SetTexture(texture)
        if mask & 8 != 0:
            eval("grid" + idx).GetCellData().SetScalars(
              triangleStripCellScalars)
            eval("mapper" + idx).SetScalarModeToUseCellData()
        if mask & 16 != 0:
            eval("grid" + idx).GetCellData().SetNormals(
              triangleStripCellNormals)

        eval("actor" + idx).AddPosition(j * 2, k * 2, 0)

        ren1.AddActor(eval("actor" + idx))
        eval("actor" + idx).GetProperty().SetRepresentationToWireframe()

        j += 1
        if (j >= 6):
            j = 0
            k += 1
        i += 1

renWin.SetSize(480, 480)

ren1.SetBackground(.7, .3, .1)
ren1.ResetCameraClippingRange()
renWin.Render()

# render the image
#
iren.Initialize()

threshold = 15

#iren.Start()
