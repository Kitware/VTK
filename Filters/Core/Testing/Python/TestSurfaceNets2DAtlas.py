#!/usr/bin/env python
"""
Test for vtkSurfaceNetsAtlas with vtkSurfaceNets2D input.

Uses a 2×2 pixel arrangement so expected values can be reasoned about analytically.

Arrangement (pixel coordinates → label):
  3  4
  1  2

Each of the 4 labeled pixels has:
  - 2 face-neighbors that are other labeled pixels  (label–label patches)
  - 2 face-neighbors that are background            (background patches)

This gives:
  - 4 background patches  (one per non-background label)
  - 4 label-label patches (the 4 face-adjacencies of the 2×2 grid: 1-2, 1-3, 2-4, 3-4)
  - 8 patches total

Tests are structured in two phases:
  Phase 1 – Atlas query API and output arrays
  Phase 2 – Rendering (4 viewports)
"""
from vtkmodules.vtkCommonCore import vtkLookupTable
from vtkmodules.vtkCommonDataModel import vtkImageData
from vtkmodules.vtkFiltersCore import vtkSurfaceNets2D, vtkSurfaceNetsAtlas
from vtkmodules.vtkFiltersExtraction import vtkExtractBlockUsingDataAssembly
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkCamera,
    vtkCompositePolyDataMapper,
    vtkRenderer,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
)

import vtkmodules.vtkInteractionStyle  # noqa: F401
import vtkmodules.vtkRenderingOpenGL2  # noqa: F401

# ---------------------------------------------------------------------------
# Build the 4-label test image (2D: z=1)
# ---------------------------------------------------------------------------
VTK_SHORT = 4
xDim, yDim, zDim = 4, 5, 1

image = vtkImageData()
image.SetDimensions(xDim, yDim, zDim)
image.AllocateScalars(VTK_SHORT, 1)

scalars = image.GetPointData().GetScalars()
scalars.Fill(0)

# 2×2 grid of labeled pixels
scalars.SetTuple1(1 + 1 * xDim, 1)
scalars.SetTuple1(2 + 1 * xDim, 2)
scalars.SetTuple1(1 + 2 * xDim, 3)
scalars.SetTuple1(2 + 2 * xDim, 4)

# ---------------------------------------------------------------------------
# Run SurfaceNets2D (no smoothing)
# ---------------------------------------------------------------------------
snets = vtkSurfaceNets2D()
snets.SetInputData(image)
for i in range(4):
    snets.SetValue(i, i + 1)
snets.GetSmoother().SetNumberOfIterations(0)
snets.Update()

surfaceMesh = snets.GetOutput()
assert surfaceMesh.GetNumberOfCells() > 0, "SurfaceNets2D produced no cells"
assert surfaceMesh.GetCellData().GetArray("BoundaryLabels") is not None, \
    "SurfaceNets2D output must carry a 'BoundaryLabels' cell-data array"

# ---------------------------------------------------------------------------
# Atlas constants
# ---------------------------------------------------------------------------
NUM_LABELS = 5   # labels 0..4 (0 = background)
NUM_PATCHES = 8  # 4 background + 4 label-label

EXTRACT_ALL = vtkSurfaceNetsAtlas.EXTRACT_ALL
EXTRACT_LABELS = vtkSurfaceNetsAtlas.EXTRACT_LABEL_SET
STYLE_ALL = vtkSurfaceNetsAtlas.OUTPUT_STYLE_ALL
STYLE_BOUNDARY = vtkSurfaceNetsAtlas.OUTPUT_STYLE_BOUNDARY


def makeAtlas(extractionMode, outputStyle, generatePatches=False):
    a = vtkSurfaceNetsAtlas()
    a.SetBackgroundLabel(0)
    a.SetInputDataObject(surfaceMesh)
    a.SetExtractionMode(extractionMode)
    a.SetOutputStyle(outputStyle)
    a.SetGeneratePatches(generatePatches)
    a.Update()
    return a


# ===========================================================================
# Phase 1 – Atlas query API and output arrays
# ===========================================================================

atlas = makeAtlas(EXTRACT_ALL, STYLE_ALL, generatePatches=True)
pdc = atlas.GetOutput()
asm = pdc.GetDataAssembly()

assert atlas.GetNumberOfLabels() == NUM_LABELS, \
    f"Expected {NUM_LABELS} labels, got {atlas.GetNumberOfLabels()}"

for lbl in range(5):
    assert atlas.HasLabel(lbl), f"Label {lbl} should be in the atlas"
assert not atlas.HasLabel(99), "Label 99 should not be in the atlas"

assert atlas.GetNumberOfPatches() == NUM_PATCHES, \
    f"Expected {NUM_PATCHES} patches, got {atlas.GetNumberOfPatches()}"

ADJACENT_PAIRS = [(1, 2), (1, 3), (2, 4), (3, 4)]
for a, b in ADJACENT_PAIRS:
    assert atlas.AreAdjacent(a, b), f"Labels {a} and {b} should be adjacent"

for lbl in range(1, 5):
    assert atlas.AreAdjacent(0, lbl), f"Background should be adjacent to label {lbl}"

regionsNode = asm.GetFirstNodeByPath("/Atlas/Regions")
assert asm.GetNumberOfChildren(regionsNode) == 4, \
    f"Expected 4 region nodes, got {asm.GetNumberOfChildren(regionsNode)}"

patchesNode = asm.GetFirstNodeByPath("/Atlas/Patches")
assert asm.GetNumberOfChildren(patchesNode) == NUM_PATCHES, \
    f"Expected {NUM_PATCHES} patch nodes, got {asm.GetNumberOfChildren(patchesNode)}"

# Spot-check region arrays (look up via assembly to get the correct PDS index)
def getPartitionByPath(pdc, assemblyPath):
    asm = pdc.GetDataAssembly()
    node = asm.GetFirstNodeByPath(assemblyPath)
    assert node >= 0, f"Assembly path {assemblyPath} not found"
    indices = list(asm.GetDataSetIndices(node, False))
    assert indices, f"No datasets at {assemblyPath}"
    return pdc.GetPartitionedDataSet(indices[0]).GetPartition(0)


regionPD = getPartitionByPath(pdc, "/Atlas/Regions/Label_1")
assert regionPD.GetCellData().GetArray("Label") is not None
assert regionPD.GetCellData().GetArray("LID") is not None
assert regionPD.GetFieldData().GetArray("AdjacentLabels") is not None
assert regionPD.GetFieldData().GetArray("PatchIDs") is not None

# Spot-check patch arrays
assert atlas.GetPatchID(1, 2) >= 0, "Patch (1,2) not found"
patchPD = getPartitionByPath(pdc, "/Atlas/Patches/Label_1_Label_2")
assert patchPD.GetCellData().GetArray("BoundaryLabels") is not None
assert patchPD.GetCellData().GetArray("PatchID") is not None

print("Phase 1: all assertions passed.")

# ===========================================================================
# Phase 2 – Rendering (4 viewports)
#   VP1 (left):         All style,      Regions
#   VP2 (centre-left):  All style,      Patches
#   VP3 (centre-right): Boundary style, Regions  (outer shell per label)
#   VP4 (right):        Boundary style, Patches  (interior label-label interfaces)
# ===========================================================================


def create_lut(n):
    lut = vtkLookupTable()
    lut.SetNumberOfColors(n + 1)
    lut.SetTableRange(0, n)
    lut.SetHueRange(0.0, 1.0)
    lut.SetSaturationRange(0.75, 0.75)
    lut.SetValueRange(0.85, 0.85)
    lut.Build()
    return lut


def extractSubtree(pdc, path):
    ext = vtkExtractBlockUsingDataAssembly()
    ext.SetInputDataObject(pdc)
    ext.SetAssemblyName("DataAssembly")
    ext.SetSelector(path)
    ext.Update()
    return ext.GetOutput()


regionLut = create_lut(NUM_LABELS)
patchLut = create_lut(NUM_PATCHES)


def regionActor(pdc):
    m = vtkCompositePolyDataMapper()
    m.SetInputDataObject(0, pdc)
    m.SetScalarModeToUseCellFieldData()
    m.SelectColorArray("Label")
    m.SetScalarVisibility(1)
    m.SetLookupTable(regionLut)
    m.SetScalarRange(0, NUM_LABELS)
    a = vtkActor()
    a.SetMapper(m)
    a.GetProperty().SetInterpolationToFlat()
    a.GetProperty().SetOpacity(0.5)
    a.GetProperty().SetLineWidth(2)
    return a


def patchActor(pdc):
    m = vtkCompositePolyDataMapper()
    m.SetInputDataObject(0, pdc)
    m.SetScalarModeToUseCellFieldData()
    m.SelectColorArray("PatchID")
    m.SetScalarVisibility(1)
    m.SetLookupTable(patchLut)
    m.SetScalarRange(0, NUM_PATCHES)
    a = vtkActor()
    a.SetMapper(m)
    a.GetProperty().SetInterpolationToFlat()
    a.GetProperty().SetLineWidth(2)
    return a


atlasAll = makeAtlas(EXTRACT_ALL, STYLE_ALL, generatePatches=True)
atlasBnd = makeAtlas(EXTRACT_ALL, STYLE_BOUNDARY, generatePatches=True)

actor1 = regionActor(extractSubtree(atlasAll.GetOutput(), "/Atlas/Regions"))
actor2 = patchActor(extractSubtree(atlasAll.GetOutput(), "/Atlas/Patches"))
actor3 = regionActor(extractSubtree(atlasBnd.GetOutput(), "/Atlas/Regions"))
actor4 = patchActor(extractSubtree(atlasBnd.GetOutput(), "/Atlas/Patches"))

ren1 = vtkRenderer()
ren1.SetBackground(0.1, 0.1, 0.1)
ren1.SetViewport(0.00, 0, 0.25, 1)

ren2 = vtkRenderer()
ren2.SetBackground(0.1, 0.1, 0.1)
ren2.SetViewport(0.25, 0, 0.50, 1)

ren3 = vtkRenderer()
ren3.SetBackground(0.1, 0.1, 0.1)
ren3.SetViewport(0.50, 0, 0.75, 1)

ren4 = vtkRenderer()
ren4.SetBackground(0.1, 0.1, 0.1)
ren4.SetViewport(0.75, 0, 1.00, 1)

renWin = vtkRenderWindow()
renWin.SetSize(800, 200)
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
renWin.AddRenderer(ren3)
renWin.AddRenderer(ren4)

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren1.AddActor(actor1)
ren2.AddActor(actor2)
ren3.AddActor(actor3)
ren4.AddActor(actor4)

# 2D: look straight down the z-axis
cam = vtkCamera()
cam.SetPosition(1.5, 1.5, 5)
cam.SetFocalPoint(1.5, 1.5, 0)
cam.SetViewUp(0, 1, 0)
ren1.SetActiveCamera(cam)
ren1.ResetCamera()
ren2.SetActiveCamera(ren1.GetActiveCamera())
ren3.SetActiveCamera(ren1.GetActiveCamera())
ren4.SetActiveCamera(ren1.GetActiveCamera())

renWin.Render()
iren.Start()
