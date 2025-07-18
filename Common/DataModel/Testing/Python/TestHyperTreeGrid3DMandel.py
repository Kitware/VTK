#!/usr/bin/env python
from vtkmodules.vtkCommonCore import (
    vtkBitArray,
    vtkDoubleArray,
    vtkLookupTable,
    vtkUnsignedCharArray,
)
from vtkmodules.vtkCommonDataModel import (
    vtkHyperTreeGrid,
    vtkHyperTreeGridNonOrientedGeometryCursor,
)
from vtkmodules.vtkFiltersGeneral import vtkShrinkFilter
from vtkmodules.vtkFiltersHyperTree import (
    vtkHyperTreeGridDepthLimiter,
    vtkHyperTreeGridGeometry,
    vtkHyperTreeGridThreshold,
    vtkHyperTreeGridToUnstructuredGrid,
)
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkCamera,
    vtkDataSetMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2

ROOT_SPLIT = 10
TARGET_LEVEL = 5
CUT_OFF = 2

# -----------------------------------------------------------------------------
# Helpers
# -----------------------------------------------------------------------------

def mandelbrotTest(x, y, timeStep = 0):
  count = 0;
  cReal = float(x);
  cImag = float(y);
  zReal = 0.0;
  zImag = float(timeStep) / 10.0;

  zReal2 = zReal * zReal;
  zImag2 = zImag * zImag;
  v1 = (zReal2 + zImag2);
  while v1 < 4.0 and count < 100:
    zImag = 2.0 * zReal * zImag + cImag
    zReal = zReal2 - zImag2 + cReal
    zReal2 = zReal * zReal
    zImag2 = zImag * zImag
    count += 1
    v1 = (zReal2 + zImag2)

  return count == 100;


def mandelbrotSide(bounds):
    count = 1
    if mandelbrotTest(bounds[0], bounds[2]):
        count += 1
    if mandelbrotTest(bounds[1], bounds[2]):
        count += 1
    if mandelbrotTest(bounds[0], bounds[3]):
        count += 1
    if mandelbrotTest(bounds[1], bounds[3]):
        count += 1
    return count


def shouldRefine(level, bounds):
    if level >= TARGET_LEVEL:
        return False
    origin = mandelbrotTest(bounds[0], bounds[2])
    originX = mandelbrotTest(bounds[1], bounds[2])
    originY = mandelbrotTest(bounds[0], bounds[3])
    originXY = mandelbrotTest(bounds[1], bounds[3])
    canRefine = bounds[4] < 0.01

    if canRefine:
        if origin and originX and originY and originXY:
            return False
        if not origin and not originX and not originY and not originXY:
            return False
        return True
    return False


def handleNode(cursor, sideArray, levelArray):
    cellBounds = [0, 0, 0, 0, 0, 0]
    level = cursor.GetLevel()
    cursor.GetBounds(cellBounds)

    # Add field
    idx = cursor.GetGlobalNodeIndex()
    side = mandelbrotSide(cellBounds)
    sideArray.InsertTuple1(idx, side)
    mask.InsertTuple1(idx, side < CUT_OFF)

    if cursor.IsLeaf():
        if shouldRefine(cursor.GetLevel(), cellBounds):
            cursor.SubdivideLeaf()
            handleNode(cursor, sideArray, mask)

    else:
        nbChildren = cursor.GetNumberOfChildren()
        for childIdx in range(nbChildren):
            cursor.ToChild(childIdx)
            handleNode(cursor, sideArray, mask)
            cursor.ToParent()


# -----------------------------------------------------------------------------
# Create Simple HTG
# -----------------------------------------------------------------------------

geoCursor = vtkHyperTreeGridNonOrientedGeometryCursor()

htg = vtkHyperTreeGrid()
htg.Initialize()
htg.SetDimensions([ROOT_SPLIT+1, ROOT_SPLIT+1, 2])
htg.SetBranchFactor(2)

sideArray = vtkUnsignedCharArray()
sideArray.SetName('sideArray')
sideArray.SetNumberOfValues(0)
sideArray.SetNumberOfComponents(1)
htg.GetCellData().AddArray(sideArray)

mask = vtkBitArray()
mask.SetName('mask')

# X[-1.75, 0.75]
xValues = vtkDoubleArray()
xValues.SetNumberOfValues(ROOT_SPLIT + 1)
for i in range(ROOT_SPLIT + 1):
    xValues.SetValue(i, -1.75 + float(i) * 0.25)
htg.SetXCoordinates(xValues)

# Y[-1.25, 1.25]
yValues = vtkDoubleArray()
yValues.SetNumberOfValues(ROOT_SPLIT + 1)
for i in range(ROOT_SPLIT + 1):
    yValues.SetValue(i, -1.25 + float(i) * 0.25)
htg.SetYCoordinates(yValues)

# Z[0, 0]
zValues = vtkDoubleArray()
zValues.SetNumberOfValues(2)
zValues.SetValue(0, 0)
zValues.SetValue(1, 0.25)
htg.SetZCoordinates(zValues)

offsetIndex = 0
for treeId in range(htg.GetMaxNumberOfTrees()):
    htg.InitializeNonOrientedGeometryCursor(geoCursor, treeId, True)
    geoCursor.SetGlobalIndexStart(offsetIndex)
    handleNode(geoCursor, sideArray, mask)
    offsetIndex += geoCursor.GetTree().GetNumberOfVertices()

print('offsetIndex: ', offsetIndex)

# Activation d'une scalaire
htg.GetCellData().SetActiveScalars('sideArray')

# DataRange sideArray on PointData HTG
dataRange = htg.GetCellData().GetArray('sideArray').GetRange()
print('sideArray on PointData HTG:', dataRange)

isFilter = False

# Depth Limiter Filter
depth = None
if True:
    print('With Depth Limiter Filter (HTG)')
    depth = vtkHyperTreeGridDepthLimiter()
    depth.SetInputData(htg)
    depth.SetDepth(5)
    isFilter = True
else:
    print('No Depth Limiter Filter (HTG)')
    depth = htg

# Threshold
threshold = None
if True:
    threshold = vtkHyperTreeGridThreshold()
    threshold.SetInputData(htg)
    threshold.SetLowerThreshold(2)
    threshold.SetUpperThreshold(dataRange[1]-1)
    threshold.Update()
    isFilter = True
else:
    print('No Threshold Filter')
    threshold = depth

showSkin = False
if showSkin:
    # Geometries
    geometry = vtkHyperTreeGridGeometry()
    if isFilter:
        geometry.SetInputConnection(threshold.GetOutputPort())
    else:
        geometry.SetInputData(htg)

    geometry.Update() # ??? Indispensable pour avoir GetRange ???

    dataRange = geometry.GetOutput().GetCellData().GetArray('sideArray').GetRange()
    print('sideArray on CellData geometry:', dataRange)

    # Shrink Filter
    if True:
        # In 3D, the shrink shouldn't be done on the geometry because it only represents the skin
        shrink = vtkShrinkFilter()
        shrink.SetInputConnection(geometry.GetOutputPort())
        shrink.SetShrinkFactor(.8)
    else:
        shrink = geometry

    shrink.Update() # ??? Indispensable pour avoir GetRange ???

    dataRange = shrink.GetOutput().GetCellData().GetArray('sideArray').GetRange()
    print('sideArray on CellData shrink:', dataRange)
else:
    # Geometries
    ns = vtkHyperTreeGridToUnstructuredGrid()
    if isFilter:
        ns.SetInputConnection(threshold.GetOutputPort())
    else:
        ns.SetInputData(htg)

    ns.Update() # ??? Indispensable pour avoir GetRange ???

    dataRange = ns.GetOutput().GetCellData().GetArray('sideArray').GetRange()
    print('sideArray on CellData ns:', dataRange)

    # Shrink Filter
    if True:
        # In 3D, the shrink shouldn't be done on the geometry because it only represents the skin
        shrink = vtkShrinkFilter()
        shrink.SetInputConnection(ns.GetOutputPort())
        shrink.SetShrinkFactor(.8)
    else:
        shrink = ns

    shrink.Update() # ??? Indispensable pour avoir GetRange ???

    dataRange = shrink.GetOutput().GetCellData().GetArray('sideArray').GetRange()
    print('sideArray on CellData shrink:', dataRange)

# LookupTable
lut = vtkLookupTable()
lut.SetHueRange(0.66, 0)
lut.Build()

# Mappers
#mapper = vtkPolyDataMapper()
mapper = vtkDataSetMapper()
mapper.SetInputConnection(shrink.GetOutputPort())

mapper.SetLookupTable(lut)
mapper.SetColorModeToMapScalars()
mapper.SetScalarModeToUseCellFieldData()
mapper.SelectColorArray('sideArray')
mapper.SetScalarRange(CUT_OFF, dataRange[1])

# Actors
actor1 = vtkActor()
actor1.SetMapper(mapper)
actor1.GetProperty().SetColor(0, 0, 0)
actor1.GetProperty().SetRepresentationToWireframe()

actor2 = vtkActor()
actor2.SetMapper(mapper)
actor2.GetProperty().EdgeVisibilityOn()
#actor2.GetProperty().SetColor(1, 1, 1)

# Camera
bd = htg.GetBounds( )
camera = vtkCamera()
camera.SetClippingRange(1., 100.)
focal = []
for i in range(3):
  focal.append( bd[ 2 * i ] + ( bd[ 2 * i + 1 ] - bd[ 2 * i] ) / 2.)
camera.SetFocalPoint( focal );
camera.SetPosition( focal[0] + 2, focal[1] + 2, focal[2] - 3. )

# Renderer
renderer = vtkRenderer()
#renderer.GetCullers().RemoveAllItems()
renderer.SetActiveCamera(camera)

renderer.AddActor(actor1)
renderer.AddActor(actor2)

# Render window
renWin = vtkRenderWindow()
renWin.AddRenderer(renderer)
renWin.SetSize(600, 400)

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# render the image
renWin.Render()
# iren.Start()

# prevent the tk window from showing up then start the event loop
# --- end of script --
