#!/usr/bin/env python
import sys
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Prevent .pyc files being created.
# Stops the vtk source being polluted
# by .pyc files.
sys.dont_write_bytecode = True

import backdrop

# Contour every cell type

# Since some of our actors are a single vertex, we need to remove all
# cullers so the single vertex actors will render
ren1 = vtk.vtkRenderer()
ren1.GetCullers().RemoveAllItems()

renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create a scene with one of each cell type
# Voxel
voxelPoints = vtk.vtkPoints()
voxelPoints.SetNumberOfPoints(8)
voxelPoints.InsertPoint(0, 0, 0, 0)
voxelPoints.InsertPoint(1, 1, 0, 0)
voxelPoints.InsertPoint(2, 0, 1, 0)
voxelPoints.InsertPoint(3, 1, 1, 0)
voxelPoints.InsertPoint(4, 0, 0, 1)
voxelPoints.InsertPoint(5, 1, 0, 1)
voxelPoints.InsertPoint(6, 0, 1, 1)
voxelPoints.InsertPoint(7, 1, 1, 1)

voxelScalars = vtk.vtkFloatArray()
voxelScalars.SetNumberOfTuples(8)
voxelScalars.InsertValue(0, 0)
voxelScalars.InsertValue(1, 1)
voxelScalars.InsertValue(2, 0)
voxelScalars.InsertValue(3, 0)
voxelScalars.InsertValue(4, 0)
voxelScalars.InsertValue(5, 0)
voxelScalars.InsertValue(6, 0)
voxelScalars.InsertValue(7, 0)

aVoxel = vtk.vtkVoxel()
aVoxel.GetPointIds().SetId(0, 0)
aVoxel.GetPointIds().SetId(1, 1)
aVoxel.GetPointIds().SetId(2, 2)
aVoxel.GetPointIds().SetId(3, 3)
aVoxel.GetPointIds().SetId(4, 4)
aVoxel.GetPointIds().SetId(5, 5)
aVoxel.GetPointIds().SetId(6, 6)
aVoxel.GetPointIds().SetId(7, 7)

aVoxelGrid = vtk.vtkUnstructuredGrid()
aVoxelGrid.Allocate(1, 1)
aVoxelGrid.InsertNextCell(aVoxel.GetCellType(), aVoxel.GetPointIds())
aVoxelGrid.SetPoints(voxelPoints)
aVoxelGrid.GetPointData().SetScalars(voxelScalars)

voxelContours = vtk.vtkContourFilter()
voxelContours.SetInputData(aVoxelGrid)
voxelContours.SetValue(0, .5)

aVoxelContourMapper = vtk.vtkDataSetMapper()
aVoxelContourMapper.SetInputConnection(voxelContours.GetOutputPort())
aVoxelContourMapper.ScalarVisibilityOff()

aVoxelMapper = vtk.vtkDataSetMapper()
aVoxelMapper.SetInputData(aVoxelGrid)
aVoxelMapper.ScalarVisibilityOff()

aVoxelActor = vtk.vtkActor()
aVoxelActor.SetMapper(aVoxelMapper)
aVoxelActor.GetProperty().SetRepresentationToWireframe()

aVoxelContourActor = vtk.vtkActor()
aVoxelContourActor.SetMapper(aVoxelContourMapper)
aVoxelContourActor.GetProperty().BackfaceCullingOn()

# Hexahedron

hexahedronPoints = vtk.vtkPoints()
hexahedronPoints.SetNumberOfPoints(8)
hexahedronPoints.InsertPoint(0, 0, 0, 0)
hexahedronPoints.InsertPoint(1, 1, 0, 0)
hexahedronPoints.InsertPoint(2, 1, 1, 0)
hexahedronPoints.InsertPoint(3, 0, 1, 0)
hexahedronPoints.InsertPoint(4, 0, 0, 1)
hexahedronPoints.InsertPoint(5, 1, 0, 1)
hexahedronPoints.InsertPoint(6, 1, 1, 1)
hexahedronPoints.InsertPoint(7, 0, 1, 1)

hexahedronScalars = vtk.vtkFloatArray()
hexahedronScalars.SetNumberOfTuples(8)
hexahedronScalars.InsertValue(0, 0)
hexahedronScalars.InsertValue(1, 1)
hexahedronScalars.InsertValue(2, 0)
hexahedronScalars.InsertValue(3, 0)
hexahedronScalars.InsertValue(4, 0)
hexahedronScalars.InsertValue(5, 0)
hexahedronScalars.InsertValue(6, 0)
hexahedronScalars.InsertValue(7, 0)

aHexahedron = vtk.vtkHexahedron()
aHexahedron.GetPointIds().SetId(0, 0)
aHexahedron.GetPointIds().SetId(1, 1)
aHexahedron.GetPointIds().SetId(2, 2)
aHexahedron.GetPointIds().SetId(3, 3)
aHexahedron.GetPointIds().SetId(4, 4)
aHexahedron.GetPointIds().SetId(5, 5)
aHexahedron.GetPointIds().SetId(6, 6)
aHexahedron.GetPointIds().SetId(7, 7)

aHexahedronGrid = vtk.vtkUnstructuredGrid()
aHexahedronGrid.Allocate(1, 1)
aHexahedronGrid.InsertNextCell(aHexahedron.GetCellType(), aHexahedron.GetPointIds())
aHexahedronGrid.SetPoints(hexahedronPoints)
aHexahedronGrid.GetPointData().SetScalars(hexahedronScalars)

hexahedronContours = vtk.vtkContourFilter()
hexahedronContours.SetInputData(aHexahedronGrid)
hexahedronContours.SetValue(0, .5)

aHexahedronContourMapper = vtk.vtkDataSetMapper()
aHexahedronContourMapper.SetInputConnection(hexahedronContours.GetOutputPort())
aHexahedronContourMapper.ScalarVisibilityOff()

aHexahedronMapper = vtk.vtkDataSetMapper()
aHexahedronMapper.SetInputData(aHexahedronGrid)
aHexahedronMapper.ScalarVisibilityOff()

aHexahedronActor = vtk.vtkActor()
aHexahedronActor.SetMapper(aHexahedronMapper)
aHexahedronActor.GetProperty().BackfaceCullingOn()
aHexahedronActor.GetProperty().SetRepresentationToWireframe()

aHexahedronContourActor = vtk.vtkActor()
aHexahedronContourActor.SetMapper(aHexahedronContourMapper)
aHexahedronContourActor.GetProperty().BackfaceCullingOn()

# Tetra

tetraPoints = vtk.vtkPoints()
tetraPoints.SetNumberOfPoints(4)
tetraPoints.InsertPoint(0, 0, 0, 0)
tetraPoints.InsertPoint(1, 1, 0, 0)
tetraPoints.InsertPoint(2, .5, 1, 0)
tetraPoints.InsertPoint(3, .5, .5, 1)

tetraScalars = vtk.vtkFloatArray()
tetraScalars.SetNumberOfTuples(4)
tetraScalars.InsertValue(0, 1)
tetraScalars.InsertValue(1, 0)
tetraScalars.InsertValue(2, 0)
tetraScalars.InsertValue(3, 0)

aTetra = vtk.vtkTetra()
aTetra.GetPointIds().SetId(0, 0)
aTetra.GetPointIds().SetId(1, 1)
aTetra.GetPointIds().SetId(2, 2)
aTetra.GetPointIds().SetId(3, 3)

aTetraGrid = vtk.vtkUnstructuredGrid()
aTetraGrid.Allocate(1, 1)
aTetraGrid.InsertNextCell(aTetra.GetCellType(), aTetra.GetPointIds())
aTetraGrid.SetPoints(tetraPoints)
aTetraGrid.GetPointData().SetScalars(tetraScalars)

tetraContours = vtk.vtkContourFilter()
tetraContours.SetInputData(aTetraGrid)
tetraContours.SetValue(0, .5)

aTetraContourMapper = vtk.vtkDataSetMapper()
aTetraContourMapper.SetInputConnection(tetraContours.GetOutputPort())
aTetraContourMapper.ScalarVisibilityOff()

aTetraMapper = vtk.vtkDataSetMapper()
aTetraMapper.SetInputData(aTetraGrid)
aTetraMapper.ScalarVisibilityOff()

aTetraContourActor = vtk.vtkActor()
aTetraContourActor.SetMapper(aTetraContourMapper)

aTetraActor = vtk.vtkActor()
aTetraActor.SetMapper(aTetraMapper)
aTetraActor.GetProperty().SetRepresentationToWireframe()

# Wedge

wedgePoints = vtk.vtkPoints()
wedgePoints.SetNumberOfPoints(6)
wedgePoints.InsertPoint(0, 0, 1, 0)
wedgePoints.InsertPoint(1, 0, 0, 0)
wedgePoints.InsertPoint(2, 0, .5, .5)
wedgePoints.InsertPoint(3, 1, 1, 0)
wedgePoints.InsertPoint(4, 1, 0, 0)
wedgePoints.InsertPoint(5, 1, .5, .5)

wedgeScalars = vtk.vtkFloatArray()
wedgeScalars.SetNumberOfTuples(6)
wedgeScalars.InsertValue(0, 1)
wedgeScalars.InsertValue(1, 1)
wedgeScalars.InsertValue(2, 0)
wedgeScalars.InsertValue(3, 1)
wedgeScalars.InsertValue(4, 1)
wedgeScalars.InsertValue(5, 0)

aWedge = vtk.vtkWedge()
aWedge.GetPointIds().SetId(0, 0)
aWedge.GetPointIds().SetId(1, 1)
aWedge.GetPointIds().SetId(2, 2)
aWedge.GetPointIds().SetId(3, 3)
aWedge.GetPointIds().SetId(4, 4)
aWedge.GetPointIds().SetId(5, 5)

aWedgeGrid = vtk.vtkUnstructuredGrid()
aWedgeGrid.Allocate(1, 1)
aWedgeGrid.InsertNextCell(aWedge.GetCellType(), aWedge.GetPointIds())
aWedgeGrid.SetPoints(wedgePoints)
aWedgeGrid.GetPointData().SetScalars(wedgeScalars)

wedgeContours = vtk.vtkContourFilter()
wedgeContours.SetInputData(aWedgeGrid)
wedgeContours.SetValue(0, .5)

aWedgeContourMapper = vtk.vtkDataSetMapper()
aWedgeContourMapper.SetInputConnection(wedgeContours.GetOutputPort())
aWedgeContourMapper.ScalarVisibilityOff()

aWedgeMapper = vtk.vtkDataSetMapper()
aWedgeMapper.SetInputData(aWedgeGrid)
aWedgeMapper.ScalarVisibilityOff()

aWedgeContourActor = vtk.vtkActor()
aWedgeContourActor.SetMapper(aWedgeContourMapper)

aWedgeActor = vtk.vtkActor()
aWedgeActor.SetMapper(aWedgeMapper)
aWedgeActor.GetProperty().SetRepresentationToWireframe()

# Pyramid

pyramidPoints = vtk.vtkPoints()
pyramidPoints.SetNumberOfPoints(5)
pyramidPoints.InsertPoint(0, 0, 0, 0)
pyramidPoints.InsertPoint(1, 1, 0, 0)
pyramidPoints.InsertPoint(2, 1, 1, 0)
pyramidPoints.InsertPoint(3, 0, 1, 0)
pyramidPoints.InsertPoint(4, .5, .5, 1)

pyramidScalars = vtk.vtkFloatArray()
pyramidScalars.SetNumberOfTuples(5)
pyramidScalars.InsertValue(0, 1)
pyramidScalars.InsertValue(1, 1)
pyramidScalars.InsertValue(2, 1)
pyramidScalars.InsertValue(3, 1)
pyramidScalars.InsertValue(4, 0)

aPyramid = vtk.vtkPyramid()
aPyramid.GetPointIds().SetId(0, 0)
aPyramid.GetPointIds().SetId(1, 1)
aPyramid.GetPointIds().SetId(2, 2)
aPyramid.GetPointIds().SetId(3, 3)
aPyramid.GetPointIds().SetId(4, 4)

aPyramidGrid = vtk.vtkUnstructuredGrid()
aPyramidGrid.Allocate(1, 1)
aPyramidGrid.InsertNextCell(aPyramid.GetCellType(), aPyramid.GetPointIds())
aPyramidGrid.SetPoints(pyramidPoints)
aPyramidGrid.GetPointData().SetScalars(pyramidScalars)

pyramidContours = vtk.vtkContourFilter()
pyramidContours.SetInputData(aPyramidGrid)
pyramidContours.SetValue(0, .5)

aPyramidContourMapper = vtk.vtkDataSetMapper()
aPyramidContourMapper.SetInputConnection(pyramidContours.GetOutputPort())
aPyramidContourMapper.ScalarVisibilityOff()

aPyramidMapper = vtk.vtkDataSetMapper()
aPyramidMapper.SetInputData(aPyramidGrid)
aPyramidMapper.ScalarVisibilityOff()

aPyramidContourActor = vtk.vtkActor()
aPyramidContourActor.SetMapper(aPyramidContourMapper)

aPyramidActor = vtk.vtkActor()
aPyramidActor.SetMapper(aPyramidMapper)
aPyramidActor.GetProperty().SetRepresentationToWireframe()

# Pixel

pixelPoints = vtk.vtkPoints()
pixelPoints.SetNumberOfPoints(4)
pixelPoints.InsertPoint(0, 0, 0, 0)
pixelPoints.InsertPoint(1, 1, 0, 0)
pixelPoints.InsertPoint(2, 0, 1, 0)
pixelPoints.InsertPoint(3, 1, 1, 0)

pixelScalars = vtk.vtkFloatArray()
pixelScalars.SetNumberOfTuples(4)
pixelScalars.InsertValue(0, 1)
pixelScalars.InsertValue(1, 0)
pixelScalars.InsertValue(2, 0)
pixelScalars.InsertValue(3, 0)

aPixel = vtk.vtkPixel()
aPixel.GetPointIds().SetId(0, 0)
aPixel.GetPointIds().SetId(1, 1)
aPixel.GetPointIds().SetId(2, 2)
aPixel.GetPointIds().SetId(3, 3)

aPixelGrid = vtk.vtkUnstructuredGrid()
aPixelGrid.Allocate(1, 1)
aPixelGrid.InsertNextCell(aPixel.GetCellType(), aPixel.GetPointIds())
aPixelGrid.SetPoints(pixelPoints)
aPixelGrid.GetPointData().SetScalars(pixelScalars)

pixelContours = vtk.vtkContourFilter()
pixelContours.SetInputData(aPixelGrid)
pixelContours.SetValue(0, .5)

aPixelContourMapper = vtk.vtkDataSetMapper()
aPixelContourMapper.SetInputConnection(pixelContours.GetOutputPort())
aPixelContourMapper.ScalarVisibilityOff()

aPixelMapper = vtk.vtkDataSetMapper()
aPixelMapper.SetInputData(aPixelGrid)
aPixelMapper.ScalarVisibilityOff()

aPixelContourActor = vtk.vtkActor()
aPixelContourActor.SetMapper(aPixelContourMapper)

aPixelActor = vtk.vtkActor()
aPixelActor.SetMapper(aPixelMapper)
aPixelActor.GetProperty().BackfaceCullingOn()
aPixelActor.GetProperty().SetRepresentationToWireframe()

# Quad

quadPoints = vtk.vtkPoints()
quadPoints.SetNumberOfPoints(4)
quadPoints.InsertPoint(0, 0, 0, 0)
quadPoints.InsertPoint(1, 1, 0, 0)
quadPoints.InsertPoint(2, 1, 1, 0)
quadPoints.InsertPoint(3, 0, 1, 0)

quadScalars = vtk.vtkFloatArray()
quadScalars.SetNumberOfTuples(4)
quadScalars.InsertValue(0, 1)
quadScalars.InsertValue(1, 0)
quadScalars.InsertValue(2, 0)
quadScalars.InsertValue(3, 0)

aQuad = vtk.vtkQuad()
aQuad.GetPointIds().SetId(0, 0)
aQuad.GetPointIds().SetId(1, 1)
aQuad.GetPointIds().SetId(2, 2)
aQuad.GetPointIds().SetId(3, 3)

aQuadGrid = vtk.vtkUnstructuredGrid()
aQuadGrid.Allocate(1, 1)
aQuadGrid.InsertNextCell(aQuad.GetCellType(), aQuad.GetPointIds())
aQuadGrid.SetPoints(quadPoints)
aQuadGrid.GetPointData().SetScalars(quadScalars)

quadContours = vtk.vtkContourFilter()
quadContours.SetInputData(aQuadGrid)
quadContours.SetValue(0, .5)

aQuadContourMapper = vtk.vtkDataSetMapper()
aQuadContourMapper.SetInputConnection(quadContours.GetOutputPort())
aQuadContourMapper.ScalarVisibilityOff()

aQuadMapper = vtk.vtkDataSetMapper()
aQuadMapper.SetInputData(aQuadGrid)
aQuadMapper.ScalarVisibilityOff()

aQuadContourActor = vtk.vtkActor()
aQuadContourActor.SetMapper(aQuadContourMapper)

aQuadActor = vtk.vtkActor()
aQuadActor.SetMapper(aQuadMapper)
aQuadActor.GetProperty().BackfaceCullingOn()
aQuadActor.GetProperty().SetRepresentationToWireframe()

# Triangle

trianglePoints = vtk.vtkPoints()
trianglePoints.SetNumberOfPoints(3)
trianglePoints.InsertPoint(0, 0, 0, 0)
trianglePoints.InsertPoint(1, 1, 0, 0)
trianglePoints.InsertPoint(2, .5, .5, 0)

triangleScalars = vtk.vtkFloatArray()
triangleScalars.SetNumberOfTuples(3)
triangleScalars.InsertValue(0, 1)
triangleScalars.InsertValue(1, 0)
triangleScalars.InsertValue(2, 0)

aTriangle = vtk.vtkTriangle()
aTriangle.GetPointIds().SetId(0, 0)
aTriangle.GetPointIds().SetId(1, 1)
aTriangle.GetPointIds().SetId(2, 2)

aTriangleGrid = vtk.vtkUnstructuredGrid()
aTriangleGrid.Allocate(1, 1)
aTriangleGrid.InsertNextCell(aTriangle.GetCellType(), aTriangle.GetPointIds())
aTriangleGrid.SetPoints(trianglePoints)
aTriangleGrid.GetPointData().SetScalars(triangleScalars)

triangleContours = vtk.vtkContourFilter()
triangleContours.SetInputData(aTriangleGrid)
triangleContours.SetValue(0, .5)

aTriangleContourMapper = vtk.vtkDataSetMapper()
aTriangleContourMapper.SetInputConnection(triangleContours.GetOutputPort())
aTriangleContourMapper.ScalarVisibilityOff()

aTriangleContourActor = vtk.vtkActor()
aTriangleContourActor.SetMapper(aTriangleContourMapper)

aTriangleMapper = vtk.vtkDataSetMapper()
aTriangleMapper.SetInputData(aTriangleGrid)
aTriangleMapper.ScalarVisibilityOff()

aTriangleActor = vtk.vtkActor()
aTriangleActor.SetMapper(aTriangleMapper)
aTriangleActor.GetProperty().BackfaceCullingOn()
aTriangleActor.GetProperty().SetRepresentationToWireframe()

# Polygon

polygonPoints = vtk.vtkPoints()
polygonPoints.SetNumberOfPoints(4)
polygonPoints.InsertPoint(0, 0, 0, 0)
polygonPoints.InsertPoint(1, 1, 0, 0)
polygonPoints.InsertPoint(2, 1, 1, 0)
polygonPoints.InsertPoint(3, 0, 1, 0)

polygonScalars = vtk.vtkFloatArray()
polygonScalars.SetNumberOfTuples(4)
polygonScalars.InsertValue(0, 1)
polygonScalars.InsertValue(1, 0)
polygonScalars.InsertValue(2, 0)
polygonScalars.InsertValue(3, 0)

aPolygon = vtk.vtkPolygon()
aPolygon.GetPointIds().SetNumberOfIds(4)
aPolygon.GetPointIds().SetId(0, 0)
aPolygon.GetPointIds().SetId(1, 1)
aPolygon.GetPointIds().SetId(2, 2)
aPolygon.GetPointIds().SetId(3, 3)

aPolygonGrid = vtk.vtkUnstructuredGrid()
aPolygonGrid.Allocate(1, 1)
aPolygonGrid.InsertNextCell(aPolygon.GetCellType(), aPolygon.GetPointIds())
aPolygonGrid.SetPoints(polygonPoints)
aPolygonGrid.GetPointData().SetScalars(polygonScalars)

polygonContours = vtk.vtkContourFilter()
polygonContours.SetInputData(aPolygonGrid)
polygonContours.SetValue(0, .5)

aPolygonContourMapper = vtk.vtkDataSetMapper()
aPolygonContourMapper.SetInputConnection(polygonContours.GetOutputPort())
aPolygonContourMapper.ScalarVisibilityOff()

aPolygonMapper = vtk.vtkDataSetMapper()
aPolygonMapper.SetInputData(aPolygonGrid)
aPolygonMapper.ScalarVisibilityOff()

aPolygonContourActor = vtk.vtkActor()
aPolygonContourActor.SetMapper(aPolygonContourMapper)

aPolygonActor = vtk.vtkActor()
aPolygonActor.SetMapper(aPolygonMapper)
aPolygonActor.GetProperty().BackfaceCullingOn()
aPolygonActor.GetProperty().SetRepresentationToWireframe()

# Triangle strip

triangleStripPoints = vtk.vtkPoints()
triangleStripPoints.SetNumberOfPoints(5)
triangleStripPoints.InsertPoint(0, 0, 1, 0)
triangleStripPoints.InsertPoint(1, 0, 0, 0)
triangleStripPoints.InsertPoint(2, 1, 1, 0)
triangleStripPoints.InsertPoint(3, 1, 0, 0)
triangleStripPoints.InsertPoint(4, 2, 1, 0)

triangleStripScalars = vtk.vtkFloatArray()
triangleStripScalars.SetNumberOfTuples(5)
triangleStripScalars.InsertValue(0, 1)
triangleStripScalars.InsertValue(1, 0)
triangleStripScalars.InsertValue(2, 0)
triangleStripScalars.InsertValue(3, 0)
triangleStripScalars.InsertValue(4, 0)

aTriangleStrip = vtk.vtkTriangleStrip()
aTriangleStrip.GetPointIds().SetNumberOfIds(5)
aTriangleStrip.GetPointIds().SetId(0, 0)
aTriangleStrip.GetPointIds().SetId(1, 1)
aTriangleStrip.GetPointIds().SetId(2, 2)
aTriangleStrip.GetPointIds().SetId(3, 3)
aTriangleStrip.GetPointIds().SetId(4, 4)

aTriangleStripGrid = vtk.vtkUnstructuredGrid()
aTriangleStripGrid.Allocate(1, 1)
aTriangleStripGrid.InsertNextCell(aTriangleStrip.GetCellType(), aTriangleStrip.GetPointIds())
aTriangleStripGrid.SetPoints(triangleStripPoints)
aTriangleStripGrid.GetPointData().SetScalars(triangleStripScalars)

aTriangleStripMapper = vtk.vtkDataSetMapper()
aTriangleStripMapper.SetInputData(aTriangleStripGrid)
aTriangleStripMapper.ScalarVisibilityOff()

triangleStripContours = vtk.vtkContourFilter()
triangleStripContours.SetInputData(aTriangleStripGrid)
triangleStripContours.SetValue(0, .5)

aTriangleStripContourMapper = vtk.vtkDataSetMapper()
aTriangleStripContourMapper.SetInputConnection(triangleStripContours.GetOutputPort())
aTriangleStripContourMapper.ScalarVisibilityOff()

aTriangleStripContourActor = vtk.vtkActor()
aTriangleStripContourActor.SetMapper(aTriangleStripContourMapper)

aTriangleStripActor = vtk.vtkActor()
aTriangleStripActor.SetMapper(aTriangleStripMapper)
aTriangleStripActor.GetProperty().BackfaceCullingOn()
aTriangleStripActor.GetProperty().SetRepresentationToWireframe()

# Line

linePoints = vtk.vtkPoints()
linePoints.SetNumberOfPoints(2)
linePoints.InsertPoint(0, 0, 0, 0)
linePoints.InsertPoint(1, 1, 1, 0)
lineScalars = vtk.vtkFloatArray()
lineScalars.SetNumberOfTuples(2)
lineScalars.InsertValue(0, 1)
lineScalars.InsertValue(1, 0)

aLine = vtk.vtkLine()
aLine.GetPointIds().SetId(0, 0)
aLine.GetPointIds().SetId(1, 1)
aLineGrid = vtk.vtkUnstructuredGrid()

aLineGrid.Allocate(1, 1)
aLineGrid.InsertNextCell(aLine.GetCellType(), aLine.GetPointIds())
aLineGrid.SetPoints(linePoints)
aLineGrid.GetPointData().SetScalars(lineScalars)

lineContours = vtk.vtkContourFilter()
lineContours.SetInputData(aLineGrid)
lineContours.SetValue(0, .5)

aLineContourMapper = vtk.vtkDataSetMapper()
aLineContourMapper.SetInputConnection(lineContours.GetOutputPort())
aLineContourMapper.ScalarVisibilityOff()

aLineContourActor = vtk.vtkActor()
aLineContourActor.SetMapper(aLineContourMapper)

aLineMapper = vtk.vtkDataSetMapper()
aLineMapper.SetInputData(aLineGrid)
aLineMapper.ScalarVisibilityOff()

aLineActor = vtk.vtkActor()
aLineActor.SetMapper(aLineMapper)
aLineActor.GetProperty().BackfaceCullingOn()
aLineActor.GetProperty().SetRepresentationToWireframe()

# Polyline

polyLinePoints = vtk.vtkPoints()
polyLinePoints.SetNumberOfPoints(3)
polyLinePoints.InsertPoint(0, 0, 0, 0)
polyLinePoints.InsertPoint(1, 1, 1, 0)
polyLinePoints.InsertPoint(2, 1, 0, 0)

polyLineScalars = vtk.vtkFloatArray()
polyLineScalars.SetNumberOfTuples(3)
polyLineScalars.InsertValue(0, 1)
polyLineScalars.InsertValue(1, 0)
polyLineScalars.InsertValue(2, 0)

aPolyLine = vtk.vtkPolyLine()
aPolyLine.GetPointIds().SetNumberOfIds(3)
aPolyLine.GetPointIds().SetId(0, 0)
aPolyLine.GetPointIds().SetId(1, 1)
aPolyLine.GetPointIds().SetId(2, 2)

aPolyLineGrid = vtk.vtkUnstructuredGrid()
aPolyLineGrid.Allocate(1, 1)
aPolyLineGrid.InsertNextCell(aPolyLine.GetCellType(), aPolyLine.GetPointIds())
aPolyLineGrid.SetPoints(polyLinePoints)
aPolyLineGrid.GetPointData().SetScalars(polyLineScalars)

polyLineContours = vtk.vtkContourFilter()
polyLineContours.SetInputData(aPolyLineGrid)
polyLineContours.SetValue(0, .5)

aPolyLineContourMapper = vtk.vtkDataSetMapper()
aPolyLineContourMapper.SetInputConnection(polyLineContours.GetOutputPort())
aPolyLineContourMapper.ScalarVisibilityOff()

aPolyLineContourActor = vtk.vtkActor()
aPolyLineContourActor.SetMapper(aPolyLineContourMapper)

aPolyLineMapper = vtk.vtkDataSetMapper()
aPolyLineMapper.SetInputData(aPolyLineGrid)
aPolyLineMapper.ScalarVisibilityOff()

aPolyLineActor = vtk.vtkActor()
aPolyLineActor.SetMapper(aPolyLineMapper)
aPolyLineActor.GetProperty().BackfaceCullingOn()
aPolyLineActor.GetProperty().SetRepresentationToWireframe()

# Vertex

vertexPoints = vtk.vtkPoints()
vertexPoints.SetNumberOfPoints(1)
vertexPoints.InsertPoint(0, 0, 0, 0)

vertexScalars = vtk.vtkFloatArray()
vertexScalars.SetNumberOfTuples(1)
vertexScalars.InsertValue(0, 1)

aVertex = vtk.vtkVertex()
aVertex.GetPointIds().SetId(0, 0)

aVertexGrid = vtk.vtkUnstructuredGrid()
aVertexGrid.Allocate(1, 1)
aVertexGrid.InsertNextCell(aVertex.GetCellType(), aVertex.GetPointIds())
aVertexGrid.SetPoints(vertexPoints)
aVertexGrid.GetPointData().SetScalars(vertexScalars)

vertexContours = vtk.vtkContourFilter()
vertexContours.SetInputData(aVertexGrid)
vertexContours.SetValue(0, 1)

aVertexContourMapper = vtk.vtkDataSetMapper()
aVertexContourMapper.SetInputConnection(vertexContours.GetOutputPort())
aVertexContourMapper.ScalarVisibilityOff()

aVertexContourActor = vtk.vtkActor()
aVertexContourActor.SetMapper(aVertexContourMapper)
aVertexContourActor.GetProperty().SetRepresentationToWireframe()

aVertexMapper = vtk.vtkDataSetMapper()
aVertexMapper.SetInputData(aVertexGrid)
aVertexMapper.ScalarVisibilityOff()

aVertexActor = vtk.vtkActor()
aVertexActor.SetMapper(aVertexMapper)
aVertexActor.GetProperty().BackfaceCullingOn()

# Poly Vertex

polyVertexPoints = vtk.vtkPoints()
polyVertexPoints.SetNumberOfPoints(3)
polyVertexPoints.InsertPoint(0, 0, 0, 0)
polyVertexPoints.InsertPoint(1, 1, 0, 0)
polyVertexPoints.InsertPoint(2, 1, 1, 0)

polyVertexScalars = vtk.vtkFloatArray()
polyVertexScalars.SetNumberOfTuples(3)
polyVertexScalars.InsertValue(0, 1)
polyVertexScalars.InsertValue(1, 0)
polyVertexScalars.InsertValue(2, 0)

aPolyVertex = vtk.vtkPolyVertex()
aPolyVertex.GetPointIds().SetNumberOfIds(3)
aPolyVertex.GetPointIds().SetId(0, 0)
aPolyVertex.GetPointIds().SetId(1, 1)
aPolyVertex.GetPointIds().SetId(2, 2)

aPolyVertexGrid = vtk.vtkUnstructuredGrid()
aPolyVertexGrid.Allocate(1, 1)
aPolyVertexGrid.InsertNextCell(aPolyVertex.GetCellType(), aPolyVertex.GetPointIds())
aPolyVertexGrid.SetPoints(polyVertexPoints)
aPolyVertexGrid.GetPointData().SetScalars(polyVertexScalars)

polyVertexContours = vtk.vtkContourFilter()
polyVertexContours.SetInputData(aPolyVertexGrid)
polyVertexContours.SetValue(0, 0)

aPolyVertexContourMapper = vtk.vtkDataSetMapper()
aPolyVertexContourMapper.SetInputConnection(polyVertexContours.GetOutputPort())
aPolyVertexContourMapper.ScalarVisibilityOff()

aPolyVertexContourActor = vtk.vtkActor()
aPolyVertexContourActor.SetMapper(aPolyVertexContourMapper)
aPolyVertexContourActor.GetProperty().SetRepresentationToWireframe()

aPolyVertexMapper = vtk.vtkDataSetMapper()
aPolyVertexMapper.SetInputData(aPolyVertexGrid)
aPolyVertexMapper.ScalarVisibilityOff()

aPolyVertexActor = vtk.vtkActor()
aPolyVertexActor.SetMapper(aPolyVertexMapper)

# Pentagonal prism

pentaPoints = vtk.vtkPoints()
pentaPoints.SetNumberOfPoints(10)
pentaPoints.InsertPoint(0, 0.25, 0.0, 0.0)
pentaPoints.InsertPoint(1, 0.75, 0.0, 0.0)
pentaPoints.InsertPoint(2, 1.0, 0.5, 0.0)
pentaPoints.InsertPoint(3, 0.5, 1.0, 0.0)
pentaPoints.InsertPoint(4, 0.0, 0.5, 0.0)
pentaPoints.InsertPoint(5, 0.25, 0.0, 1.0)
pentaPoints.InsertPoint(6, 0.75, 0.0, 1.0)
pentaPoints.InsertPoint(7, 1.0, 0.5, 1.0)
pentaPoints.InsertPoint(8, 0.5, 1.0, 1.0)
pentaPoints.InsertPoint(9, 0.0, 0.5, 1.0)

pentaScalars = vtk.vtkFloatArray()
pentaScalars.SetNumberOfTuples(10)
pentaScalars.InsertValue(0, 1)
pentaScalars.InsertValue(1, 1)
pentaScalars.InsertValue(2, 1)
pentaScalars.InsertValue(3, 1)
pentaScalars.InsertValue(4, 1)
pentaScalars.InsertValue(5, 0)
pentaScalars.InsertValue(6, 0)
pentaScalars.InsertValue(7, 0)
pentaScalars.InsertValue(8, 0)
pentaScalars.InsertValue(9, 0)

aPenta = vtk.vtkPentagonalPrism()
aPenta.GetPointIds().SetId(0, 0)
aPenta.GetPointIds().SetId(1, 1)
aPenta.GetPointIds().SetId(2, 2)
aPenta.GetPointIds().SetId(3, 3)
aPenta.GetPointIds().SetId(4, 4)
aPenta.GetPointIds().SetId(5, 5)
aPenta.GetPointIds().SetId(6, 6)
aPenta.GetPointIds().SetId(7, 7)
aPenta.GetPointIds().SetId(8, 8)
aPenta.GetPointIds().SetId(9, 9)

aPentaGrid = vtk.vtkUnstructuredGrid()
aPentaGrid.Allocate(1, 1)
aPentaGrid.InsertNextCell(aPenta.GetCellType(), aPenta.GetPointIds())
aPentaGrid.SetPoints(pentaPoints)
aPentaGrid.GetPointData().SetScalars(pentaScalars)

pentaContours = vtk.vtkContourFilter()
pentaContours.SetInputData(aPentaGrid)
pentaContours.SetValue(0, .5)

aPentaContourMapper = vtk.vtkDataSetMapper()
aPentaContourMapper.SetInputConnection(pentaContours.GetOutputPort())
aPentaContourMapper.ScalarVisibilityOff()

aPentaMapper = vtk.vtkDataSetMapper()
aPentaMapper.SetInputData(aPentaGrid)
aPentaMapper.ScalarVisibilityOff()

aPentaActor = vtk.vtkActor()
aPentaActor.SetMapper(aPentaMapper)
aPentaActor.GetProperty().BackfaceCullingOn()
aPentaActor.GetProperty().SetRepresentationToWireframe()

aPentaContourActor = vtk.vtkActor()
aPentaContourActor.SetMapper(aPentaContourMapper)
aPentaContourActor.GetProperty().BackfaceCullingOn()

# Hexagonal prism

hexaPoints = vtk.vtkPoints()
hexaPoints.SetNumberOfPoints(12)
hexaPoints.InsertPoint(0, 0.0, 0.0, 0.0)
hexaPoints.InsertPoint(1, 0.5, 0.0, 0.0)
hexaPoints.InsertPoint(2, 1.0, 0.5, 0.0)
hexaPoints.InsertPoint(3, 1.0, 1.0, 0.0)
hexaPoints.InsertPoint(4, 0.5, 1.0, 0.0)
hexaPoints.InsertPoint(5, 0.0, 0.5, 0.0)
hexaPoints.InsertPoint(6, 0.0, 0.0, 1.0)
hexaPoints.InsertPoint(7, 0.5, 0.0, 1.0)
hexaPoints.InsertPoint(8, 1.0, 0.5, 1.0)
hexaPoints.InsertPoint(9, 1.0, 1.0, 1.0)
hexaPoints.InsertPoint(10, 0.5, 1.0, 1.0)
hexaPoints.InsertPoint(11, 0.0, 0.5, 1.0)

hexaScalars = vtk.vtkFloatArray()
hexaScalars.SetNumberOfTuples(12)
hexaScalars.InsertValue(0, 1)
hexaScalars.InsertValue(1, 1)
hexaScalars.InsertValue(2, 1)
hexaScalars.InsertValue(3, 1)
hexaScalars.InsertValue(4, 1)
hexaScalars.InsertValue(5, 1)
hexaScalars.InsertValue(6, 0)
hexaScalars.InsertValue(7, 0)
hexaScalars.InsertValue(8, 0)
hexaScalars.InsertValue(9, 0)
hexaScalars.InsertValue(10, 0)
hexaScalars.InsertValue(11, 0)

aHexa = vtk.vtkHexagonalPrism()
aHexa.GetPointIds().SetId(0, 0)
aHexa.GetPointIds().SetId(1, 1)
aHexa.GetPointIds().SetId(2, 2)
aHexa.GetPointIds().SetId(3, 3)
aHexa.GetPointIds().SetId(4, 4)
aHexa.GetPointIds().SetId(5, 5)
aHexa.GetPointIds().SetId(6, 6)
aHexa.GetPointIds().SetId(7, 7)
aHexa.GetPointIds().SetId(8, 8)
aHexa.GetPointIds().SetId(9, 9)
aHexa.GetPointIds().SetId(10, 10)
aHexa.GetPointIds().SetId(11, 11)

aHexaGrid = vtk.vtkUnstructuredGrid()
aHexaGrid.Allocate(1, 1)
aHexaGrid.InsertNextCell(aHexa.GetCellType(), aHexa.GetPointIds())
aHexaGrid.SetPoints(hexaPoints)
aHexaGrid.GetPointData().SetScalars(hexaScalars)

hexaContours = vtk.vtkContourFilter()
hexaContours.SetInputData(aHexaGrid)
hexaContours.SetValue(0, .5)

aHexaContourMapper = vtk.vtkDataSetMapper()
aHexaContourMapper.SetInputConnection(hexaContours.GetOutputPort())
aHexaContourMapper.ScalarVisibilityOff()

aHexaMapper = vtk.vtkDataSetMapper()
aHexaMapper.SetInputData(aHexaGrid)
aHexaMapper.ScalarVisibilityOff()

aHexaActor = vtk.vtkActor()
aHexaActor.SetMapper(aHexaMapper)
aHexaActor.GetProperty().BackfaceCullingOn()
aHexaActor.GetProperty().SetRepresentationToWireframe()

aHexaContourActor = vtk.vtkActor()
aHexaContourActor.SetMapper(aHexaContourMapper)
aHexaContourActor.GetProperty().BackfaceCullingOn()

ren1.SetBackground(.1, .2, .3)
renWin.SetSize(400, 400)

ren1.AddActor(aVoxelActor)
aVoxelActor.GetProperty().SetDiffuseColor(1, 0, 0)

ren1.AddActor(aVoxelContourActor)
aVoxelContourActor.GetProperty().SetDiffuseColor(1, 0, 0)

ren1.AddActor(aHexahedronActor)
aHexahedronActor.GetProperty().SetDiffuseColor(1, 1, 0)

ren1.AddActor(aHexahedronContourActor)
aHexahedronContourActor.GetProperty().SetDiffuseColor(1, 1, 0)

ren1.AddActor(aTetraActor)
aTetraActor.GetProperty().SetDiffuseColor(0, 1, 0)

ren1.AddActor(aTetraContourActor)
aTetraContourActor.GetProperty().SetDiffuseColor(0, 1, 0)

ren1.AddActor(aWedgeActor)
aWedgeActor.GetProperty().SetDiffuseColor(0, 1, 1)

ren1.AddActor(aWedgeContourActor)
aWedgeContourActor.GetProperty().SetDiffuseColor(0, 1, 1)

ren1.AddActor(aPyramidActor)
aPyramidActor.GetProperty().SetDiffuseColor(1, 0, 1)

ren1.AddActor(aPyramidContourActor)
aPyramidContourActor.GetProperty().SetDiffuseColor(1, 0, 1)

ren1.AddActor(aPixelActor)
aPixelActor.GetProperty().SetDiffuseColor(0, 1, 1)

ren1.AddActor(aPixelContourActor)
aPixelContourActor.GetProperty().SetDiffuseColor(0, 1, 1)

ren1.AddActor(aQuadActor)
aQuadActor.GetProperty().SetDiffuseColor(1, 0, 1)

ren1.AddActor(aQuadContourActor)
aQuadContourActor.GetProperty().SetDiffuseColor(1, 0, 1)

ren1.AddActor(aTriangleActor)
aTriangleActor.GetProperty().SetDiffuseColor(.3, 1, .5)

ren1.AddActor(aTriangleContourActor)
aTriangleContourActor.GetProperty().SetDiffuseColor(.3, 1, .5)

ren1.AddActor(aPolygonActor)
aPolygonActor.GetProperty().SetDiffuseColor(1, .4, .5)

ren1.AddActor(aPolygonContourActor)
aPolygonContourActor.GetProperty().SetDiffuseColor(1, .4, .5)

ren1.AddActor(aTriangleStripActor)
aTriangleStripActor.GetProperty().SetDiffuseColor(.3, .7, 1)

ren1.AddActor(aTriangleStripContourActor)
aTriangleStripContourActor.GetProperty().SetDiffuseColor(.3, .7, 1)

ren1.AddActor(aLineActor)
aLineActor.GetProperty().SetDiffuseColor(.2, 1, 1)

ren1.AddActor(aLineContourActor)
aLineContourActor.GetProperty().SetDiffuseColor(.2, 1, 1)

ren1.AddActor(aPolyLineActor)
aPolyLineActor.GetProperty().SetDiffuseColor(1, 1, 1)

ren1.AddActor(aPolyLineContourActor)
aPolyLineContourActor.GetProperty().SetDiffuseColor(1, 1, 1)

ren1.AddActor(aVertexActor)
aVertexActor.GetProperty().SetDiffuseColor(1, 1, 1)

ren1.AddActor(aVertexContourActor)
aVertexContourActor.GetProperty().SetDiffuseColor(1, 1, 1)

ren1.AddActor(aPolyVertexActor)
aPolyVertexActor.GetProperty().SetDiffuseColor(1, 1, 1)

ren1.AddActor(aPolyVertexContourActor)
aPolyVertexContourActor.GetProperty().SetDiffuseColor(1, 1, 1)

ren1.AddActor(aPentaActor)
aPentaActor.GetProperty().SetDiffuseColor(.2, .4, .7)

ren1.AddActor(aPentaContourActor)
aPentaContourActor.GetProperty().SetDiffuseColor(.2, .4, .7)

ren1.AddActor(aHexaActor)
aHexaActor.GetProperty().SetDiffuseColor(.7, .5, 1)

ren1.AddActor(aHexaContourActor)
aHexaContourActor.GetProperty().SetDiffuseColor(.7, .5, 1)

# places everyone!!

aVoxelContourActor.AddPosition(0, 0, 0)
aVoxelContourActor.AddPosition(0, 2, 0)
aHexahedronContourActor.AddPosition(2, 0, 0)
aHexahedronContourActor.AddPosition(0, 2, 0)
aHexahedronActor.AddPosition(2, 0, 0)
aTetraContourActor.AddPosition(4, 0, 0)
aTetraContourActor.AddPosition(0, 2, 0)
aTetraActor.AddPosition(4, 0, 0)
aWedgeContourActor.AddPosition(6, 0, 0)
aWedgeContourActor.AddPosition(0, 2, 0)
aWedgeActor.AddPosition(6, 0, 0)
aPyramidContourActor.AddPosition(8, 0, 0)
aPyramidContourActor.AddPosition(0, 2, 0)
aPyramidActor.AddPosition(8, 0, 0)

aPixelContourActor.AddPosition(0, 4, 0)
aPixelContourActor.AddPosition(0, 2, 0)
aPixelActor.AddPosition(0, 4, 0)
aQuadContourActor.AddPosition(2, 4, 0)
aQuadContourActor.AddPosition(0, 2, 0)
aQuadActor.AddPosition(2, 4, 0)
aTriangleContourActor.AddPosition(4, 4, 0)
aTriangleContourActor.AddPosition(0, 2, 0)
aTriangleActor.AddPosition(4, 4, 0)
aPolygonContourActor.AddPosition(6, 4, 0)
aPolygonContourActor.AddPosition(0, 2, 0)
aPolygonActor.AddPosition(6, 4, 0)
aTriangleStripContourActor.AddPosition(8, 4, 0)
aTriangleStripContourActor.AddPosition(0, 2, 0)
aTriangleStripActor.AddPosition(8, 4, 0)

aLineContourActor.AddPosition(0, 8, 0)
aLineContourActor.AddPosition(0, 2, 0)
aLineActor.AddPosition(0, 8, 0)
aPolyLineContourActor.AddPosition(2, 8, 0)
aPolyLineContourActor.AddPosition(0, 2, 0)
aPolyLineActor.AddPosition(2, 8, 0)

aVertexContourActor.AddPosition(0, 12, 0)
aVertexContourActor.AddPosition(0, 2, 0)
aVertexActor.AddPosition(0, 12, 0)
aPolyVertexContourActor.AddPosition(2, 12, 0)
aPolyVertexContourActor.AddPosition(0, 2, 0)
aPolyVertexActor.AddPosition(2, 12, 0)

aPentaContourActor.AddPosition(4, 8, 0)
aPentaContourActor.AddPosition(0, 2, 0)
aPentaActor.AddPosition(4, 8, 0)
aHexaContourActor.AddPosition(6, 8, 0)
aHexaContourActor.AddPosition(0, 2, 0)
aHexaActor.AddPosition(6, 8, 0)

[base, back, left] = backdrop.BuildBackdrop(-1, 11, -1, 16, -1, 2, .1)

ren1.AddActor(base)
base.GetProperty().SetDiffuseColor(.2, .2, .2)
ren1.AddActor(left)
left.GetProperty().SetDiffuseColor(.2, .2, .2)
ren1.AddActor(back)
back.GetProperty().SetDiffuseColor(.2, .2, .2)

ren1.ResetCamera()
ren1.GetActiveCamera().Dolly(1.5)
ren1.ResetCameraClippingRange()

renWin.Render()

# render the image
#
iren.Initialize()
#iren.Start()
