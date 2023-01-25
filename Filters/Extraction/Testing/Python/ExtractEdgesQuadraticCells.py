#!/usr/bin/env python
from vtkmodules.vtkCommonCore import (
    vtkFloatArray,
    vtkPoints,
)
from vtkmodules.vtkCommonDataModel import (
    vtkBiQuadraticQuad,
    vtkBiQuadraticQuadraticHexahedron,
    vtkBiQuadraticQuadraticWedge,
    vtkQuadraticHexahedron,
    vtkQuadraticLinearQuad,
    vtkQuadraticLinearWedge,
    vtkQuadraticPyramid,
    vtkQuadraticQuad,
    vtkQuadraticTetra,
    vtkQuadraticTriangle,
    vtkQuadraticWedge,
    vtkTriQuadraticHexahedron,
    vtkTriQuadraticPyramid,
    vtkUnstructuredGrid,
)
from vtkmodules.vtkFiltersCore import (
    vtkAppendFilter,
    vtkExtractEdges,
)
from vtkmodules.vtkFiltersGeneral import vtkShrinkPolyData
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkDataSetMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot

VTK_DATA_ROOT = vtkGetDataRoot()

try:
    import numpy as np
    from vtkmodules.util.numpy_support import numpy_to_vtk as ntov
except ImportError:
    print("This test requires numpy!")
    from vtkmodules.test import Testing

    Testing.skip()

# Create 2D and 3D quadratic cells and extract their edges
# Quadratic triangle
triPoints = vtkPoints()
triPoints.SetNumberOfPoints(6)
triPointsCoords = np.array([
    [2.0, 0.0, 0.0],
    [3.0, 0.0, 0.0],
    [2.5, 0.8, 0.0],
    [2.5, 0.0, 0.0],
    [2.75, 0.4, 0.0],
    [2.25, 0.4, 0.0]])
triPoints.SetData(ntov(triPointsCoords))
triScalars = vtkFloatArray()
triScalars.SetNumberOfTuples(6)
triScalars.InsertValue(0, 0.0)
triScalars.InsertValue(1, 0.0)
triScalars.InsertValue(2, 0.0)
triScalars.InsertValue(3, 1.0)
triScalars.InsertValue(4, 1.0)
triScalars.InsertValue(5, 0.0)
aTri = vtkQuadraticTriangle()
for i in range(aTri.GetNumberOfPoints()):
    aTri.GetPointIds().SetId(i, i)
aTriGrid = vtkUnstructuredGrid()
aTriGrid.Allocate(1, 1)
aTriGrid.InsertNextCell(aTri.GetCellType(), aTri.GetPointIds())
aTriGrid.SetPoints(triPoints)
aTriGrid.GetPointData().SetScalars(triScalars)

# Quadratic quadrilateral
quadPoints = vtkPoints()
quadPoints.SetNumberOfPoints(8)
quadPointsCoords = np.array([
    [4.0, 0.0, 0.0],
    [5.0, 0.0, 0.0],
    [5.0, 1.0, 0.0],
    [4.0, 1.0, 0.0],
    [4.5, 0.0, 0.0],
    [5.0, 0.5, 0.0],
    [4.5, 1.0, 0.0],
    [4.0, 0.5, 0.0]])
quadPoints.SetData(ntov(quadPointsCoords))
quadScalars = vtkFloatArray()
quadScalars.SetNumberOfTuples(8)
quadScalars.InsertValue(0, 0.0)
quadScalars.InsertValue(1, 0.0)
quadScalars.InsertValue(2, 1.0)
quadScalars.InsertValue(3, 1.0)
quadScalars.InsertValue(4, 1.0)
quadScalars.InsertValue(5, 0.0)
quadScalars.InsertValue(6, 0.0)
quadScalars.InsertValue(7, 0.0)
aQuad = vtkQuadraticQuad()
for i in range(aQuad.GetNumberOfPoints()):
    aQuad.GetPointIds().SetId(i, i)
aQuadGrid = vtkUnstructuredGrid()
aQuadGrid.Allocate(1, 1)
aQuadGrid.InsertNextCell(aQuad.GetCellType(), aQuad.GetPointIds())
aQuadGrid.SetPoints(quadPoints)
aQuadGrid.GetPointData().SetScalars(quadScalars)

# BiQuadratic quadrilateral
BquadPoints = vtkPoints()
BquadPoints.SetNumberOfPoints(9)
BquadPointsCoords = np.array([
    [4.0, 2.0, 0.0],
    [5.0, 2.0, 0.0],
    [5.0, 3.0, 0.0],
    [4.0, 3.0, 0.0],
    [4.5, 2.0, 0.0],
    [5.0, 2.5, 0.0],
    [4.5, 3.0, 0.0],
    [4.0, 2.5, 0.0],
    [4.5, 2.5, 0.0]])
BquadPoints.SetData(ntov(BquadPointsCoords))
BquadScalars = vtkFloatArray()
BquadScalars.SetNumberOfTuples(9)
BquadScalars.InsertValue(0, 1.0)
BquadScalars.InsertValue(1, 1.0)
BquadScalars.InsertValue(2, 1.0)
BquadScalars.InsertValue(3, 1.0)
BquadScalars.InsertValue(4, 0.0)
BquadScalars.InsertValue(5, 0.0)
BquadScalars.InsertValue(6, 0.0)
BquadScalars.InsertValue(7, 0.0)
BquadScalars.InsertValue(8, 0.0)
BQuad = vtkBiQuadraticQuad()
for i in range(BQuad.GetNumberOfPoints()):
    BQuad.GetPointIds().SetId(i, i)
BQuadGrid = vtkUnstructuredGrid()
BQuadGrid.Allocate(1, 1)
BQuadGrid.InsertNextCell(BQuad.GetCellType(), BQuad.GetPointIds())
BQuadGrid.SetPoints(BquadPoints)
BQuadGrid.GetPointData().SetScalars(BquadScalars)

# Quadratic linear quadrilateral
QLquadPoints = vtkPoints()
QLquadPoints.SetNumberOfPoints(6)
QLquadPointsCoords = np.array([
    [4.0, 4.0, 0.0],
    [5.0, 4.0, 0.0],
    [5.0, 5.0, 0.0],
    [4.0, 5.0, 0.0],
    [4.5, 4.0, 0.0],
    [4.5, 5.0, 0.0]])
QLquadPoints.SetData(ntov(QLquadPointsCoords))
QLquadScalars = vtkFloatArray()
QLquadScalars.SetNumberOfTuples(6)
QLquadScalars.InsertValue(0, 1.0)
QLquadScalars.InsertValue(1, 1.0)
QLquadScalars.InsertValue(2, 1.0)
QLquadScalars.InsertValue(3, 1.0)
QLquadScalars.InsertValue(4, 0.0)
QLquadScalars.InsertValue(5, 0.0)
QLQuad = vtkQuadraticLinearQuad()
for i in range(QLQuad.GetNumberOfPoints()):
    QLQuad.GetPointIds().SetId(i, i)
QLQuadGrid = vtkUnstructuredGrid()
QLQuadGrid.Allocate(1, 1)
QLQuadGrid.InsertNextCell(QLQuad.GetCellType(), QLQuad.GetPointIds())
QLQuadGrid.SetPoints(QLquadPoints)
QLQuadGrid.GetPointData().SetScalars(QLquadScalars)

# Quadratic tetrahedron
tetPoints = vtkPoints()
tetPoints.SetNumberOfPoints(10)
tetPointsCoords = np.array([
    [6.0, 0.0, 0.0],
    [7.0, 0.0, 0.0],
    [6.5, 0.8, 0.0],
    [6.5, 0.4, 1.0],
    [6.5, 0.0, 0.0],
    [6.75, 0.4, 0.0],
    [6.25, 0.4, 0.0],
    [6.25, 0.2, 0.5],
    [6.75, 0.2, 0.5],
    [6.50, 0.6, 0.5]])
tetPoints.SetData(ntov(tetPointsCoords))
tetScalars = vtkFloatArray()
tetScalars.SetNumberOfTuples(10)
tetScalars.InsertValue(0, 1.0)
tetScalars.InsertValue(1, 1.0)
tetScalars.InsertValue(2, 1.0)
tetScalars.InsertValue(3, 1.0)
tetScalars.InsertValue(4, 0.0)
tetScalars.InsertValue(5, 0.0)
tetScalars.InsertValue(6, 0.0)
tetScalars.InsertValue(7, 0.0)
tetScalars.InsertValue(8, 0.0)
tetScalars.InsertValue(9, 0.0)
aTet = vtkQuadraticTetra()
for i in range(aTet.GetNumberOfPoints()):
    aTet.GetPointIds().SetId(i, i)
aTetGrid = vtkUnstructuredGrid()
aTetGrid.Allocate(1, 1)
aTetGrid.InsertNextCell(aTet.GetCellType(), aTet.GetPointIds())
aTetGrid.SetPoints(tetPoints)
aTetGrid.GetPointData().SetScalars(tetScalars)

# Quadratic hexahedron
hexPoints = vtkPoints()
hexPoints.SetNumberOfPoints(20)
hexPointsCoords = np.array([
    [8, 0, 0],
    [9, 0, 0],
    [9, 1, 0],
    [8, 1, 0],
    [8, 0, 1],
    [9, 0, 1],
    [9, 1, 1],
    [8, 1, 1],
    [8.5, 0, 0],
    [9, 0.5, 0],
    [8.5, 1, 0],
    [8, 0.5, 0],
    [8.5, 0, 1],
    [9, 0.5, 1],
    [8.5, 1, 1],
    [8, 0.5, 1],
    [8, 0, 0.5],
    [9, 0, 0.5],
    [9, 1, 0.5],
    [8, 1, 0.5]])
hexPoints.SetData(ntov(hexPointsCoords))
hexScalars = vtkFloatArray()
hexScalars.SetNumberOfTuples(20)
hexScalars.InsertValue(0, 1.0)
hexScalars.InsertValue(1, 1.0)
hexScalars.InsertValue(2, 1.0)
hexScalars.InsertValue(3, 1.0)
hexScalars.InsertValue(4, 1.0)
hexScalars.InsertValue(5, 1.0)
hexScalars.InsertValue(6, 1.0)
hexScalars.InsertValue(7, 1.0)
hexScalars.InsertValue(8, 0.0)
hexScalars.InsertValue(9, 0.0)
hexScalars.InsertValue(10, 0.0)
hexScalars.InsertValue(11, 0.0)
hexScalars.InsertValue(12, 0.0)
hexScalars.InsertValue(13, 0.0)
hexScalars.InsertValue(14, 0.0)
hexScalars.InsertValue(15, 0.0)
hexScalars.InsertValue(16, 0.0)
hexScalars.InsertValue(17, 0.0)
hexScalars.InsertValue(18, 0.0)
hexScalars.InsertValue(19, 0.0)
aHex = vtkQuadraticHexahedron()
for i in range(aHex.GetNumberOfPoints()):
    aHex.GetPointIds().SetId(i, i)
aHexGrid = vtkUnstructuredGrid()
aHexGrid.Allocate(1, 1)
aHexGrid.InsertNextCell(aHex.GetCellType(), aHex.GetPointIds())
aHexGrid.SetPoints(hexPoints)
aHexGrid.GetPointData().SetScalars(hexScalars)

# TriQuadratic hexahedron
TQhexPoints = vtkPoints()
TQhexPoints.SetNumberOfPoints(27)
TQhexPointsCoords = np.array([
    [8, 2, 0],
    [9, 2, 0],
    [9, 3, 0],
    [8, 3, 0],
    [8, 2, 1],
    [9, 2, 1],
    [9, 3, 1],
    [8, 3, 1],
    [8.5, 2, 0],
    [9, 2.5, 0],
    [8.5, 3, 0],
    [8, 2.5, 0],
    [8.5, 2, 1],
    [9, 2.5, 1],
    [8.5, 3, 1],
    [8, 2.5, 1],
    [8, 2, 0.5],
    [9, 2, 0.5],
    [9, 3, 0.5],
    [8, 3, 0.5],
    [8, 2.5, 0.5],
    [9, 2.5, 0.5],
    [8.5, 2, 0.5],
    [8.5, 3, 0.5],
    [8.5, 2.5, 0.0],
    [8.5, 2.5, 1],
    [8.5, 2.5, 0.5]])
TQhexPoints.SetData(ntov(TQhexPointsCoords))
TQhexScalars = vtkFloatArray()
TQhexScalars.SetNumberOfTuples(27)
TQhexScalars.InsertValue(0, 1.0)
TQhexScalars.InsertValue(1, 1.0)
TQhexScalars.InsertValue(2, 1.0)
TQhexScalars.InsertValue(3, 1.0)
TQhexScalars.InsertValue(4, 1.0)
TQhexScalars.InsertValue(5, 1.0)
TQhexScalars.InsertValue(6, 1.0)
TQhexScalars.InsertValue(7, 1.0)
TQhexScalars.InsertValue(8, 0.0)
TQhexScalars.InsertValue(9, 0.0)
TQhexScalars.InsertValue(10, 0.0)
TQhexScalars.InsertValue(11, 0.0)
TQhexScalars.InsertValue(12, 0.0)
TQhexScalars.InsertValue(13, 0.0)
TQhexScalars.InsertValue(14, 0.0)
TQhexScalars.InsertValue(15, 0.0)
TQhexScalars.InsertValue(16, 0.0)
TQhexScalars.InsertValue(17, 0.0)
TQhexScalars.InsertValue(18, 0.0)
TQhexScalars.InsertValue(19, 0.0)
TQhexScalars.InsertValue(20, 0.0)
TQhexScalars.InsertValue(21, 0.0)
TQhexScalars.InsertValue(22, 0.0)
TQhexScalars.InsertValue(23, 0.0)
TQhexScalars.InsertValue(24, 0.0)
TQhexScalars.InsertValue(25, 0.0)
TQhexScalars.InsertValue(26, 0.0)
TQHex = vtkTriQuadraticHexahedron()
for i in range(TQHex.GetNumberOfPoints()):
    TQHex.GetPointIds().SetId(i, i)
TQHexGrid = vtkUnstructuredGrid()
TQHexGrid.Allocate(1, 1)
TQHexGrid.InsertNextCell(TQHex.GetCellType(), TQHex.GetPointIds())
TQHexGrid.SetPoints(TQhexPoints)
TQHexGrid.GetPointData().SetScalars(TQhexScalars)

# BiQuadratic Quadratic hexahedron
BQhexPoints = vtkPoints()
BQhexPoints.SetNumberOfPoints(24)
BQhexPointsCoords = np.array([
    [8, 4, 0],
    [9, 4, 0],
    [9, 5, 0],
    [8, 5, 0],
    [8, 4, 1],
    [9, 4, 1],
    [9, 5, 1],
    [8, 5, 1],
    [8.5, 4, 0],
    [9, 4.5, 0],
    [8.5, 5, 0],
    [8, 4.5, 0],
    [8.5, 4, 1],
    [9, 4.5, 1],
    [8.5, 5, 1],
    [8, 4.5, 1],
    [8, 4, 0.5],
    [9, 4, 0.5],
    [9, 5, 0.5],
    [8, 5, 0.5],
    [8, 4.5, 0.5],
    [9, 4.5, 0.5],
    [8.5, 4, 0.5],
    [8.5, 5, 0.5]])
BQhexPoints.SetData(ntov(BQhexPointsCoords))
BQhexScalars = vtkFloatArray()
BQhexScalars.SetNumberOfTuples(24)
BQhexScalars.InsertValue(0, 1.0)
BQhexScalars.InsertValue(1, 1.0)
BQhexScalars.InsertValue(2, 1.0)
BQhexScalars.InsertValue(3, 1.0)
BQhexScalars.InsertValue(4, 1.0)
BQhexScalars.InsertValue(5, 1.0)
BQhexScalars.InsertValue(6, 1.0)
BQhexScalars.InsertValue(7, 1.0)
BQhexScalars.InsertValue(8, 0.0)
BQhexScalars.InsertValue(9, 0.0)
BQhexScalars.InsertValue(10, 0.0)
BQhexScalars.InsertValue(11, 0.0)
BQhexScalars.InsertValue(12, 0.0)
BQhexScalars.InsertValue(13, 0.0)
BQhexScalars.InsertValue(14, 0.0)
BQhexScalars.InsertValue(15, 0.0)
BQhexScalars.InsertValue(16, 0.0)
BQhexScalars.InsertValue(17, 0.0)
BQhexScalars.InsertValue(18, 0.0)
BQhexScalars.InsertValue(19, 0.0)
BQhexScalars.InsertValue(20, 0.0)
BQhexScalars.InsertValue(21, 0.0)
BQhexScalars.InsertValue(22, 0.0)
BQhexScalars.InsertValue(23, 0.0)
BQHex = vtkBiQuadraticQuadraticHexahedron()
for i in range(BQHex.GetNumberOfPoints()):
    BQHex.GetPointIds().SetId(i, i)
BQHexGrid = vtkUnstructuredGrid()
BQHexGrid.Allocate(1, 1)
BQHexGrid.InsertNextCell(BQHex.GetCellType(), BQHex.GetPointIds())
BQHexGrid.SetPoints(BQhexPoints)
BQHexGrid.GetPointData().SetScalars(BQhexScalars)

# Quadratic wedge
wedgePoints = vtkPoints()
wedgePoints.SetNumberOfPoints(15)
wedgePointsCoords = np.array([
    [10, 0, 0],
    [11, 0, 0],
    [10, 1, 0],
    [10, 0, 1],
    [11, 0, 1],
    [10, 1, 1],
    [10.5, 0, 0],
    [10.5, 0.5, 0],
    [10, 0.5, 0],
    [10.5, 0, 1],
    [10.5, 0.5, 1],
    [10, 0.5, 1],
    [10, 0, 0.5],
    [11, 0, 0.5],
    [10, 1, 0.5]])
wedgePoints.SetData(ntov(wedgePointsCoords))
wedgeScalars = vtkFloatArray()
wedgeScalars.SetNumberOfTuples(15)
wedgeScalars.InsertValue(0, 1.0)
wedgeScalars.InsertValue(1, 1.0)
wedgeScalars.InsertValue(2, 1.0)
wedgeScalars.InsertValue(3, 1.0)
wedgeScalars.InsertValue(4, 1.0)
wedgeScalars.InsertValue(5, 1.0)
wedgeScalars.InsertValue(6, 0.0)
wedgeScalars.InsertValue(7, 0.0)
wedgeScalars.InsertValue(8, 0.0)
wedgeScalars.InsertValue(9, 0.0)
wedgeScalars.InsertValue(10, 0.0)
wedgeScalars.InsertValue(11, 0.0)
wedgeScalars.InsertValue(12, 0.0)
wedgeScalars.InsertValue(13, 0.0)
wedgeScalars.InsertValue(14, 0.0)
aWedge = vtkQuadraticWedge()
for i in range(aWedge.GetNumberOfPoints()):
    aWedge.GetPointIds().SetId(i, i)
aWedgeGrid = vtkUnstructuredGrid()
aWedgeGrid.Allocate(1, 1)
aWedgeGrid.InsertNextCell(aWedge.GetCellType(), aWedge.GetPointIds())
aWedgeGrid.SetPoints(wedgePoints)
aWedgeGrid.GetPointData().SetScalars(wedgeScalars)

# Quadratic linear wedge
QLwedgePoints = vtkPoints()
QLwedgePoints.SetNumberOfPoints(12)
QLwedgePointsCoords = np.array([
    [10, 4, 0],
    [11, 4, 0],
    [10, 5, 0],
    [10, 4, 1],
    [11, 4, 1],
    [10, 5, 1],
    [10.5, 4, 0],
    [10.5, 4.5, 0],
    [10, 4.5, 0],
    [10.5, 4, 1],
    [10.5, 4.5, 1],
    [10, 4.5, 1]])
QLwedgePoints.SetData(ntov(QLwedgePointsCoords))
QLwedgeScalars = vtkFloatArray()
QLwedgeScalars.SetNumberOfTuples(12)
QLwedgeScalars.InsertValue(0, 1.0)
QLwedgeScalars.InsertValue(1, 1.0)
QLwedgeScalars.InsertValue(2, 1.0)
QLwedgeScalars.InsertValue(3, 1.0)
QLwedgeScalars.InsertValue(4, 1.0)
QLwedgeScalars.InsertValue(5, 1.0)
QLwedgeScalars.InsertValue(6, 0.0)
QLwedgeScalars.InsertValue(7, 0.0)
QLwedgeScalars.InsertValue(8, 0.0)
QLwedgeScalars.InsertValue(9, 0.0)
QLwedgeScalars.InsertValue(10, 0.0)
QLwedgeScalars.InsertValue(11, 0.0)
QLWedge = vtkQuadraticLinearWedge()
for i in range(QLWedge.GetNumberOfPoints()):
    QLWedge.GetPointIds().SetId(i, i)
QLWedgeGrid = vtkUnstructuredGrid()
QLWedgeGrid.Allocate(1, 1)
QLWedgeGrid.InsertNextCell(QLWedge.GetCellType(), QLWedge.GetPointIds())
QLWedgeGrid.SetPoints(QLwedgePoints)
QLWedgeGrid.GetPointData().SetScalars(QLwedgeScalars)

# BiQuadratic wedge
BQwedgePoints = vtkPoints()
BQwedgePoints.SetNumberOfPoints(18)
BQwedgePointsCoords = np.array([
    [10, 2, 0],
    [11, 2, 0],
    [10, 3, 0],
    [10, 2, 1],
    [11, 2, 1],
    [10, 3, 1],
    [10.5, 2, 0],
    [10.5, 2.5, 0],
    [10, 2.5, 0],
    [10.5, 2, 1],
    [10.5, 2.5, 1],
    [10, 2.5, 1],
    [10, 2, 0.5],
    [11, 2, 0.5],
    [10, 3, 0.5],
    [10.5, 2, 0.5],
    [10.5, 2.5, 0.5],
    [10, 2.5, 0.5]])
BQwedgePoints.SetData(ntov(BQwedgePointsCoords))
BQwedgeScalars = vtkFloatArray()
BQwedgeScalars.SetNumberOfTuples(18)
BQwedgeScalars.InsertValue(0, 1.0)
BQwedgeScalars.InsertValue(1, 1.0)
BQwedgeScalars.InsertValue(2, 1.0)
BQwedgeScalars.InsertValue(3, 1.0)
BQwedgeScalars.InsertValue(4, 1.0)
BQwedgeScalars.InsertValue(5, 1.0)
BQwedgeScalars.InsertValue(6, 0.0)
BQwedgeScalars.InsertValue(7, 0.0)
BQwedgeScalars.InsertValue(8, 0.0)
BQwedgeScalars.InsertValue(9, 0.0)
BQwedgeScalars.InsertValue(10, 0.0)
BQwedgeScalars.InsertValue(11, 0.0)
BQwedgeScalars.InsertValue(12, 0.0)
BQwedgeScalars.InsertValue(13, 0.0)
BQwedgeScalars.InsertValue(14, 0.0)
BQwedgeScalars.InsertValue(15, 0.0)
BQwedgeScalars.InsertValue(16, 0.0)
BQwedgeScalars.InsertValue(17, 0.0)
BQWedge = vtkBiQuadraticQuadraticWedge()
for i in range(BQWedge.GetNumberOfPoints()):
    BQWedge.GetPointIds().SetId(i, i)
BQWedgeGrid = vtkUnstructuredGrid()
BQWedgeGrid.Allocate(1, 1)
BQWedgeGrid.InsertNextCell(BQWedge.GetCellType(), BQWedge.GetPointIds())
BQWedgeGrid.SetPoints(BQwedgePoints)
BQWedgeGrid.GetPointData().SetScalars(BQwedgeScalars)

# Quadratic pyramid
pyraPoints = vtkPoints()
pyraPoints.SetNumberOfPoints(13)
pyraPointsCoords = np.array([
    [12, 0, 0],
    [13, 0, 0],
    [13, 1, 0],
    [12, 1, 0],
    [12, 0, 1],
    [12.5, 0, 0],
    [13, 0.5, 0],
    [12.5, 1, 0],
    [12, 0.5, 0],
    [12, 0, 0.5],
    [12.5, 0, 0.5],
    [12.5, 0.5, 0.5],
    [12, 0.5, 0.5]])
pyraPoints.SetData(ntov(pyraPointsCoords))
pyraScalars = vtkFloatArray()
pyraScalars.SetNumberOfTuples(13)
pyraScalars.InsertValue(0, 1.0)
pyraScalars.InsertValue(1, 1.0)
pyraScalars.InsertValue(2, 1.0)
pyraScalars.InsertValue(3, 1.0)
pyraScalars.InsertValue(4, 1.0)
pyraScalars.InsertValue(5, 1.0)
pyraScalars.InsertValue(6, 0.0)
pyraScalars.InsertValue(7, 0.0)
pyraScalars.InsertValue(8, 0.0)
pyraScalars.InsertValue(9, 0.0)
pyraScalars.InsertValue(10, 0.0)
pyraScalars.InsertValue(11, 0.0)
pyraScalars.InsertValue(12, 0.0)
aPyramid = vtkQuadraticPyramid()
for i in range(aPyramid.GetNumberOfPoints()):
    aPyramid.GetPointIds().SetId(i, i)
aPyramidGrid = vtkUnstructuredGrid()
aPyramidGrid.Allocate(1, 1)
aPyramidGrid.InsertNextCell(aPyramid.GetCellType(), aPyramid.GetPointIds())
aPyramidGrid.SetPoints(pyraPoints)
aPyramidGrid.GetPointData().SetScalars(pyraScalars)

# TriQuadratic pyramid
TQpyraPoints = vtkPoints()
TQpyraPoints.SetNumberOfPoints(19)
TQpyraPointsCoords = np.array([
    [12, 2, 0],
    [13, 2, 0],
    [13, 3, 0],
    [12, 3, 0],
    [12, 2, 1],
    [12.5, 2, 0],
    [13, 2.5, 0],
    [12.5, 3, 0],
    [12, 2.5, 0],
    [12, 2, 0.5],
    [12.5, 2, 0.5],
    [12.5, 2.5, 0.5],
    [12, 2.5, 0.5],
    [12.5, 2.5, 0],
    [12 + 1.0 / 3.0, 2, 1.0 / 3.0],
    [12 + 2.0 / 3.0, 2 + 1.0 / 3.0, 1.0 / 3.0],
    [12 + 1.0 / 3.0, 2 + 2.0 / 3.0, 1.0 / 3.0],
    [12, 2 + 1.0 / 3.0, 1.0 / 3.0],
    [12 + 0.4, 2 + 0.4, 0.2]])
TQpyraPoints.SetData(ntov(TQpyraPointsCoords))
TQpyraScalars = vtkFloatArray()
TQpyraScalars.SetNumberOfTuples(19)
TQpyraScalars.InsertValue(0, 1.0)
TQpyraScalars.InsertValue(1, 1.0)
TQpyraScalars.InsertValue(2, 1.0)
TQpyraScalars.InsertValue(3, 1.0)
TQpyraScalars.InsertValue(4, 1.0)
TQpyraScalars.InsertValue(5, 1.0)
TQpyraScalars.InsertValue(6, 0.0)
TQpyraScalars.InsertValue(7, 0.0)
TQpyraScalars.InsertValue(8, 0.0)
TQpyraScalars.InsertValue(9, 0.0)
TQpyraScalars.InsertValue(10, 0.0)
TQpyraScalars.InsertValue(11, 0.0)
TQpyraScalars.InsertValue(12, 0.0)
TQpyraScalars.InsertValue(13, 0.0)
TQpyraScalars.InsertValue(14, 0.0)
TQpyraScalars.InsertValue(15, 0.0)
TQpyraScalars.InsertValue(16, 0.0)
TQpyraScalars.InsertValue(17, 0.0)
TQpyraScalars.InsertValue(18, 1.0)
aTQPyramid = vtkTriQuadraticPyramid()
for i in range(aTQPyramid.GetNumberOfPoints()):
    aTQPyramid.GetPointIds().SetId(i, i)
aTQPyramidGrid = vtkUnstructuredGrid()
aTQPyramidGrid.Allocate(1, 1)
aTQPyramidGrid.InsertNextCell(aTQPyramid.GetCellType(), aTQPyramid.GetPointIds())
aTQPyramidGrid.SetPoints(TQpyraPoints)
aTQPyramidGrid.GetPointData().SetScalars(TQpyraScalars)

# Append the quadratic cells together
appendF = vtkAppendFilter()
appendF.AddInputData(BQuadGrid)
appendF.AddInputData(QLQuadGrid)
appendF.AddInputData(QLWedgeGrid)
appendF.AddInputData(aTriGrid)
appendF.AddInputData(aQuadGrid)
appendF.AddInputData(aTetGrid)
appendF.AddInputData(aHexGrid)
appendF.AddInputData(TQHexGrid)
appendF.AddInputData(BQHexGrid)
appendF.AddInputData(aWedgeGrid)
appendF.AddInputData(BQWedgeGrid)
appendF.AddInputData(aPyramidGrid)
appendF.AddInputData(aTQPyramidGrid)

# Extract the edges
extract = vtkExtractEdges()
extract.SetInputConnection(appendF.GetOutputPort())
shrink = vtkShrinkPolyData()
shrink.SetInputConnection(extract.GetOutputPort())
shrink.SetShrinkFactor(0.90)
aMapper = vtkDataSetMapper()
aMapper.SetInputConnection(shrink.GetOutputPort())
aActor = vtkActor()
aActor.SetMapper(aMapper)
aActor.GetProperty().SetRepresentationToWireframe()

# Create the rendering related stuff.
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
ren1.SetBackground(.1, .2, .3)

renWin.SetSize(400, 200)

# specify properties
ren1.AddActor(aActor)
renWin.Render()

ren1.GetActiveCamera().Dolly(2.0)
ren1.ResetCameraClippingRange()
# render the image
#
iren.Initialize()
# iren.Start()
