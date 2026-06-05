#!/usr/bin/env python
"""
Comprehensive test for vtkSurfaceNetsAtlas.

Uses the same 8-label 2×2×2 voxel arrangement as TestSurfaceNets3D3.py so
expected values can be reasoned about analytically.

Arrangement (voxel coordinates → label):

  z=1 plane:   z=2 plane:
    3  4          7  8
    1  2          5  6

Each of the 8 labeled voxels has:
  - 3 face-neighbors that are other labeled voxels  (label–label patches)
  - 3 face-neighbors that are background            (background patches)

This gives:
  - 8  background patches  (one per non-background label)
  - 12 label-label patches (the 12 face-adjacencies of the 2×2×2 cube)
  - 20 patches total

Tests are structured in three phases:
  Phase 1 – Atlas query API  (label, adjacency, patch queries)
  Phase 2 – Extraction modes and output styles
  Phase 3 – Rendering (3 viewports)
"""
from vtkmodules.vtkCommonCore import vtkLookupTable
from vtkmodules.vtkCommonDataModel import vtkImageData
from vtkmodules.vtkFiltersCore import vtkSurfaceNets3D, vtkSurfaceNetsAtlas
from vtkmodules.vtkFiltersExtraction import vtkExtractBlockUsingDataAssembly
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkCamera,
    vtkCompositePolyDataMapper,
    vtkRenderer,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
)

# Ensure the rendering backend and default interactor styles are registered.
import vtkmodules.vtkInteractionStyle  # noqa: F401
import vtkmodules.vtkRenderingOpenGL2  # noqa: F401

# ---------------------------------------------------------------------------
# Build the 8-label test volume
# ---------------------------------------------------------------------------
VTK_SHORT = 4
xDim, yDim, zDim = 4, 5, 6
sliceSize = xDim * yDim

image = vtkImageData()
image.SetDimensions(xDim, yDim, zDim)
image.AllocateScalars(VTK_SHORT, 1)


def GenerateIndex(i, j, k):
    return i + j * xDim + k * sliceSize


scalars = image.GetPointData().GetScalars()
scalars.Fill(0)

# 2×2×2 cube of labeled voxels
scalars.SetTuple1(GenerateIndex(1, 1, 1), 1)
scalars.SetTuple1(GenerateIndex(2, 1, 1), 2)
scalars.SetTuple1(GenerateIndex(1, 2, 1), 3)
scalars.SetTuple1(GenerateIndex(2, 2, 1), 4)
scalars.SetTuple1(GenerateIndex(1, 1, 2), 5)
scalars.SetTuple1(GenerateIndex(2, 1, 2), 6)
scalars.SetTuple1(GenerateIndex(1, 2, 2), 7)
scalars.SetTuple1(GenerateIndex(2, 2, 2), 8)

# ---------------------------------------------------------------------------
# Run SurfaceNets3D (no smoothing, quads)
# ---------------------------------------------------------------------------
snets = vtkSurfaceNets3D()
snets.SetInputData(image)
for i in range(8):
    snets.SetValue(i, i + 1)
snets.GetSmoother().SetNumberOfIterations(0)
snets.Update()

surfaceMesh = snets.GetOutput()
print(f"Surface net: {surfaceMesh.GetNumberOfPoints()} pts, "
      f"{surfaceMesh.GetNumberOfCells()} cells")

# ===========================================================================
# Phase 1 – Atlas query API
# ===========================================================================

# Build once with patches so all internal tables are populated.
atlasQ = vtkSurfaceNetsAtlas()
atlasQ.SetBackgroundLabel(0)
atlasQ.SetInputDataObject(surfaceMesh)
atlasQ.SetGeneratePatches(True)
atlasQ.Update()

NUM_LABELS = 9  # labels 0..8 (0 = background)
NUM_PATCHES = 20  # 8 background + 12 label-label

# -- Label queries -----------------------------------------------------------
assert atlasQ.GetNumberOfLabels() == NUM_LABELS, \
    f"Expected {NUM_LABELS} labels, got {atlasQ.GetNumberOfLabels()}"

for lbl in range(9):
    assert atlasQ.HasLabel(lbl), f"Label {lbl} should be in the atlas"
assert not atlasQ.HasLabel(99), "Label 99 should not be in the atlas"

# LID ↔ Label round-trip (labels are assigned LIDs in sorted order)
for lbl in range(9):
    lid = atlasQ.GetLIDForLabel(lbl)
    assert lid >= 0, f"Label {lbl} has no LID"
    assert atlasQ.GetLabelForLID(lid) == lbl, \
        f"Round-trip failed for label {lbl}: LID {lid} → {atlasQ.GetLabelForLID(lid)}"

assert atlasQ.GetLIDForLabel(99) == -1, "Unknown label must return -1"
assert atlasQ.GetLabelForLID(-1) == -1, "Negative LID must return -1"
assert atlasQ.GetLabelForLID(999) == -1, "Out-of-range LID must return -1"

# -- Adjacency queries -------------------------------------------------------
# Background is adjacent to every non-background label
for lbl in range(1, 9):
    assert atlasQ.AreAdjacent(0, lbl), \
        f"Background should be adjacent to label {lbl}"
    assert atlasQ.AreAdjacent(lbl, 0), \
        f"AreAdjacent must be symmetric for ({lbl}, 0)"

# No label is adjacent to itself
for lbl in range(9):
    assert not atlasQ.AreAdjacent(lbl, lbl), \
        f"Label {lbl} must not be adjacent to itself"

# 12 label-label adjacencies (edges of the 2×2×2 cube)
#   x-edges: 1-2, 3-4, 5-6, 7-8
#   y-edges: 1-3, 2-4, 5-7, 6-8
#   z-edges: 1-5, 2-6, 3-7, 4-8
ADJACENT_PAIRS = [
    (1, 2), (3, 4), (5, 6), (7, 8),
    (1, 3), (2, 4), (5, 7), (6, 8),
    (1, 5), (2, 6), (3, 7), (4, 8),
]
for a, b in ADJACENT_PAIRS:
    assert atlasQ.AreAdjacent(a, b), f"Labels {a} and {b} should be adjacent"
    assert atlasQ.AreAdjacent(b, a), f"AreAdjacent({b},{a}) must be symmetric"

# Diagonal pairs (not face-adjacent) must NOT be adjacent
DIAGONAL_PAIRS = [(1, 4), (1, 6), (1, 7), (1, 8), (2, 3), (2, 5), (2, 7), (2, 8)]
for a, b in DIAGONAL_PAIRS:
    assert not atlasQ.AreAdjacent(a, b), \
        f"Labels {a} and {b} are diagonal and must NOT be adjacent"

# Each non-background label has exactly 4 adjacencies: 3 cube neighbours + background
for lbl in range(1, 9):
    adj = atlasQ.GetAdjacentLabels(lbl)
    assert len(adj) == 4, \
        f"Label {lbl} should have 4 adjacencies, got {len(adj)}: {list(adj)}"
    assert 0 in adj, f"Background missing from adjacency list of label {lbl}"

# Background is adjacent to all 8 non-background labels
bgAdj = atlasQ.GetAdjacentLabels(0)
assert len(bgAdj) == 8, \
    f"Background should have 8 adjacencies, got {len(bgAdj)}"
for lbl in range(1, 9):
    assert lbl in bgAdj, f"Label {lbl} missing from background adjacency list"

# Unknown label returns empty adjacency list
assert len(atlasQ.GetAdjacentLabels(99)) == 0, \
    "Unknown label must return empty adjacency list"

# -- Patch queries -----------------------------------------------------------
assert atlasQ.GetNumberOfPatches() == NUM_PATCHES, \
    f"Expected {NUM_PATCHES} patches, got {atlasQ.GetNumberOfPatches()}"

# GetPatchID is symmetric; GetPatchLabels round-trips back to the same pair
ALL_PAIRS = ADJACENT_PAIRS + [(0, lbl) for lbl in range(1, 9)]
for a, b in ALL_PAIRS:
    idx = atlasQ.GetPatchID(a, b)
    assert idx >= 0, f"No patch found for ({a},{b})"
    assert atlasQ.GetPatchID(a, b) == atlasQ.GetPatchID(b, a), \
        f"GetPatchID({a},{b}) != GetPatchID({b},{a})"
    labels = [0, 0]
    atlasQ.GetPatchLabels(idx, labels)
    assert frozenset(labels) == frozenset([a, b]), \
        f"Patch {idx} labels {labels} don't match ({a},{b})"

# Out-of-range patch index returns (-1, -1)
sentinel = [0, 0]
atlasQ.GetPatchLabels(-1, sentinel)
assert sentinel == [-1, -1], "GetPatchLabels(-1) should return [-1, -1]"
atlasQ.GetPatchLabels(9999, sentinel)
assert sentinel == [-1, -1], "GetPatchLabels(9999) should return [-1, -1]"

# Non-existent pair returns -1
assert atlasQ.GetPatchID(1, 8) == -1, "Labels 1 and 8 share no patch"
assert atlasQ.GetPatchID(99, 1) == -1, "Unknown label must give -1"

# Every patch has at least one cell; out-of-range returns 0
for p in range(NUM_PATCHES):
    assert atlasQ.GetPatchCellCount(p) > 0, f"Patch {p} has zero cells"
assert atlasQ.GetPatchCellCount(-1) == 0
assert atlasQ.GetPatchCellCount(9999) == 0

# Each non-background label touches exactly 4 patches
for lbl in range(1, 9):
    patches = atlasQ.GetPatchesForLabel(lbl)
    assert len(patches) == 4, \
        f"Label {lbl} should touch 4 patches, got {len(patches)}"

# Background touches all 8 background-label patches
assert len(atlasQ.GetPatchesForLabel(0)) == 8, \
    "Background should touch 8 patches"
assert len(atlasQ.GetPatchesForLabel(99)) == 0, \
    "Unknown label must return empty patch list"

# LIDToLabel field data must be present on the PDC root and have 9 entries
pdcQ = atlasQ.GetOutput()
lidArr = pdcQ.GetFieldData().GetArray("LIDToLabel")
assert lidArr is not None, "LIDToLabel field data array is missing"
assert lidArr.GetNumberOfValues() == NUM_LABELS, \
    f"LIDToLabel should have {NUM_LABELS} values"

patchLIDsArr = pdcQ.GetFieldData().GetArray("PatchLIDs")
assert patchLIDsArr is not None, "PatchLIDs field data array is missing"
assert patchLIDsArr.GetNumberOfTuples() == NUM_PATCHES, \
    f"PatchLIDs should have {NUM_PATCHES} tuples"

patchLabelsArr = pdcQ.GetFieldData().GetArray("PatchLabels")
assert patchLabelsArr is not None, "PatchLabels field data array is missing"
assert patchLabelsArr.GetNumberOfTuples() == NUM_PATCHES, \
    f"PatchLabels should have {NUM_PATCHES} tuples"

print("Phase 1 (atlas queries): all assertions passed.")

# ===========================================================================
# Phase 2 – Extraction modes and output styles
# ===========================================================================

EXTRACT_ALL = vtkSurfaceNetsAtlas.EXTRACT_ALL
EXTRACT_LABELS = vtkSurfaceNetsAtlas.EXTRACT_LABEL_SET
STYLE_ALL = vtkSurfaceNetsAtlas.OUTPUT_STYLE_ALL
STYLE_BOUNDARY = vtkSurfaceNetsAtlas.OUTPUT_STYLE_BOUNDARY


def makeAtlas(extractionMode=EXTRACT_ALL, outputStyle=STYLE_ALL,
              labels=None, generatePatches=False, resolveNM=False):
    a = vtkSurfaceNetsAtlas()
    a.SetBackgroundLabel(0)
    a.SetInputDataObject(surfaceMesh)
    a.SetExtractionMode(extractionMode)
    a.SetOutputStyle(outputStyle)
    a.SetGeneratePatches(generatePatches)
    a.SetResolveNonManifoldPoints(resolveNM)
    if labels:
        for lbl in labels:
            a.AddSelectedLabel(lbl)
    a.Update()
    return a


def countChildren(pdc, subtreePath):
    asm = pdc.GetDataAssembly()
    node = asm.GetFirstNodeByPath(subtreePath)
    return asm.GetNumberOfChildren(node) if node >= 0 else -1


def totalCells(pdc):
    total = 0
    for i in range(pdc.GetNumberOfPartitionedDataSets()):
        pds = pdc.GetPartitionedDataSet(i)
        for j in range(pds.GetNumberOfPartitions()):
            p = pds.GetPartition(j)
            if p:
                total += p.GetNumberOfCells()
    return total


# -- AllRegions / All style --------------------------------------------------
atlasAllAll = makeAtlas(EXTRACT_ALL, STYLE_ALL)
pdcAA = atlasAllAll.GetOutput()
# Background is never emitted as a region; only 8 non-background labels
assert pdcAA.GetNumberOfPartitionedDataSets() == 8, \
    f"AllRegions/All should produce 8 PDS, got {pdcAA.GetNumberOfPartitionedDataSets()}"
assert countChildren(pdcAA, "/Atlas/Regions") == 8, \
    "Assembly /Atlas/Regions should have 8 children"

# -- AllRegions / Boundary style ---------------------------------------------
atlasAllBnd = makeAtlas(EXTRACT_ALL, STYLE_BOUNDARY)
pdcAB = atlasAllBnd.GetOutput()
assert pdcAB.GetNumberOfPartitionedDataSets() == 8, \
    "AllRegions/Boundary should produce 8 PDS"

# Boundary mode must produce fewer cells than All mode (interior faces excluded)
cellsAll = totalCells(pdcAA)
cellsBnd = totalCells(pdcAB)
assert cellsBnd < cellsAll, \
    f"Boundary ({cellsBnd}) should be less than All ({cellsAll}) total cells"

# -- LabelSet / All style ----------------------------------------------------
atlasSelAll = makeAtlas(EXTRACT_LABELS, STYLE_ALL, labels=[1, 3, 5])
pdcSA = atlasSelAll.GetOutput()
assert pdcSA.GetNumberOfPartitionedDataSets() == 3, \
    f"LabelSet(3) should produce 3 PDS, got {pdcSA.GetNumberOfPartitionedDataSets()}"
assert countChildren(pdcSA, "/Atlas/Regions") == 3

# Empty label set → empty output (not treated as "all")
atlasEmpty = makeAtlas(EXTRACT_LABELS, STYLE_ALL)
assert atlasEmpty.GetOutput().GetNumberOfPartitionedDataSets() == 0, \
    "Empty LabelSet selection must produce empty output"

# -- GeneratePatches / All style (all 20 patches) ----------------------------
atlasPatches = makeAtlas(EXTRACT_ALL, STYLE_ALL, generatePatches=True)
pdcP = atlasPatches.GetOutput()
asmP = pdcP.GetDataAssembly()

patchesNode = asmP.GetFirstNodeByPath("/Atlas/Patches")
assert patchesNode >= 0, "Patches subtree must exist when GeneratePatches is on"
assert asmP.GetNumberOfChildren(patchesNode) == NUM_PATCHES, \
    f"Expected {NUM_PATCHES} patch nodes, got {asmP.GetNumberOfChildren(patchesNode)}"

# Verify region node attributes: Label and LID
regionNode = asmP.GetFirstNodeByPath("/Atlas/Regions/Label_1")
assert regionNode >= 0, "Assembly node /Atlas/Regions/Label_1 must exist"
assert int(asmP.GetAttributeOrDefault(regionNode, "Label", -1)) == 1
assert int(asmP.GetAttributeOrDefault(regionNode, "LID", -1)) >= 0

# Verify patch node naming and attributes: Label0, Label1, LID0, LID1
patchNode01 = asmP.GetFirstNodeByPath("/Atlas/Patches/Label_0_Label_1")
assert patchNode01 >= 0, "Patch node /Atlas/Patches/Label_0_Label_1 must exist"
lbl0 = int(asmP.GetAttributeOrDefault(patchNode01, "Label0", -1))
lbl1 = int(asmP.GetAttributeOrDefault(patchNode01, "Label1", -1))
lid0 = int(asmP.GetAttributeOrDefault(patchNode01, "LID0", -1))
lid1 = int(asmP.GetAttributeOrDefault(patchNode01, "LID1", -1))
assert frozenset([lbl0, lbl1]) == frozenset([0, 1]), \
    f"Patch Label_0_Label_1 labels are {lbl0},{lbl1} — expected 0 and 1"
assert lid0 >= 0 and lid1 >= 0, "LID attributes must be non-negative"

# Verify a label-label patch node exists
patchNode12 = asmP.GetFirstNodeByPath("/Atlas/Patches/Label_1_Label_2")
assert patchNode12 >= 0, "Patch node /Atlas/Patches/Label_1_Label_2 must exist"


# -- Partition metadata arrays -----------------------------------------------
# Retrieve region Label_1 and patch Label_0_Label_1 from pdcP.
def getPartition(pdc, assemblyPath):
    asm = pdc.GetDataAssembly()
    node = asm.GetFirstNodeByPath(assemblyPath)
    assert node >= 0, f"Assembly path {assemblyPath} not found"
    indices = list(asm.GetDataSetIndices(node, False))
    assert indices, f"No datasets at {assemblyPath}"
    return pdc.GetPartitionedDataSet(indices[0]).GetPartition(0)


region1 = getPartition(pdcP, "/Atlas/Regions/Label_1")

# "Label" — constant cell-data, value must equal 1 (same for every cell)
labelCD = region1.GetCellData().GetArray("Label")
assert labelCD is not None, "Region 'Label' cell-data array missing"
assert labelCD.GetValue(0) == 1, f"Region Label_1 'Label' must be 1, got {labelCD.GetValue(0)}"

# "LID" — constant cell-data, non-negative (same for every cell)
lidCD = region1.GetCellData().GetArray("LID")
assert lidCD is not None, "Region 'LID' cell-data array missing"
assert lidCD.GetValue(0) >= 0, "Region 'LID' must be non-negative"

# "AdjacentLabels" — field-data, 4 neighbours (3 cube + background)
adjArr = region1.GetFieldData().GetArray("AdjacentLabels")
assert adjArr is not None, "Region 'AdjacentLabels' field-data array missing"
assert adjArr.GetNumberOfValues() == 4, \
    f"Label 1 should have 4 adjacent labels, got {adjArr.GetNumberOfValues()}"
assert 0 in [adjArr.GetValue(i) for i in range(4)], \
    "Background (0) must appear in region AdjacentLabels"

# "PatchIDs" — field-data, 4 entries
patchIDsFD = region1.GetFieldData().GetArray("PatchIDs")
assert patchIDsFD is not None, "Region 'PatchIDs' field-data array missing"
assert patchIDsFD.GetNumberOfValues() == 4, \
    f"Label 1 should touch 4 patches, got {patchIDsFD.GetNumberOfValues()}"

patch01 = getPartition(pdcP, "/Atlas/Patches/Label_0_Label_1")

# "BoundaryLabels" — 2-component cell-data matching [0, 1]
blCD = patch01.GetCellData().GetArray("BoundaryLabels")
assert blCD is not None, "Patch 'BoundaryLabels' cell-data array missing"
assert blCD.GetNumberOfComponents() == 2, "'BoundaryLabels' must have 2 components"
assert frozenset([blCD.GetComponent(0, 0), blCD.GetComponent(0, 1)]) == frozenset([0, 1]), \
    f"'BoundaryLabels' tuple must be [0,1], got [{blCD.GetComponent(0, 0)},{blCD.GetComponent(0, 1)}]"

# "PatchID" — constant cell-data matching the atlas patch ID (same for every cell)
piCD = patch01.GetCellData().GetArray("PatchID")
assert piCD is not None, "Patch 'PatchID' cell-data array missing"
expectedPI = atlasPatches.GetPatchID(0, 1)
assert piCD.GetValue(0) == expectedPI, \
    f"Patch 'PatchID' should be {expectedPI}, got {piCD.GetValue(0)}"

# -- GeneratePatches / Boundary style (only 8 interior label-label patches) --
atlasBndPatches = makeAtlas(EXTRACT_ALL, STYLE_BOUNDARY, generatePatches=True)
pdcBP = atlasBndPatches.GetOutput()
asmBP = pdcBP.GetDataAssembly()
patchesNodeBP = asmBP.GetFirstNodeByPath("/Atlas/Patches")
assert asmBP.GetNumberOfChildren(patchesNodeBP) == 12, \
    f"Boundary/GeneratePatches should emit 12 interior label-label patches, " \
    f"got {asmBP.GetNumberOfChildren(patchesNodeBP)}"

# -- GenerateRegions=False (patches-only) ------------------------------------
# With regions off and all 20 patches on, no region PDS should appear.
atlasPatchesOnly = vtkSurfaceNetsAtlas()
atlasPatchesOnly.SetBackgroundLabel(0)
atlasPatchesOnly.SetInputDataObject(surfaceMesh)
atlasPatchesOnly.SetOutputStyleToAll()
atlasPatchesOnly.GenerateRegionsOff()
atlasPatchesOnly.GeneratePatchesOn()
atlasPatchesOnly.Update()
pdcPO = atlasPatchesOnly.GetOutput()
asmPO = pdcPO.GetDataAssembly()
assert asmPO.GetFirstNodeByPath("/Atlas/Regions") < 0, \
    "Regions subtree must be absent when GenerateRegions is off"
assert asmPO.GetNumberOfChildren(asmPO.GetFirstNodeByPath("/Atlas/Patches")) == NUM_PATCHES, \
    f"Patches-only output should have {NUM_PATCHES} patch nodes"
assert pdcPO.GetNumberOfPartitionedDataSets() == NUM_PATCHES, \
    f"Patches-only output should have {NUM_PATCHES} PDS, got {pdcPO.GetNumberOfPartitionedDataSets()}"
assert pdcPO.GetFieldData().GetArray("PatchLIDs") is not None, \
    "PatchLIDs must always be present"
assert pdcPO.GetFieldData().GetArray("PatchLabels") is not None, \
    "PatchLabels must always be present"

# -- GeneratePatches=False: PatchLIDs and PatchLabels still present ----------
atlasRegionsOnly = makeAtlas(EXTRACT_ALL, STYLE_ALL, generatePatches=False)
assert atlasRegionsOnly.GetOutput().GetFieldData().GetArray("PatchLIDs") is not None, \
    "PatchLIDs must always be present (even when GeneratePatches is off)"
assert atlasRegionsOnly.GetOutput().GetFieldData().GetArray("PatchLabels") is not None, \
    "PatchLabels must always be present (even when GeneratePatches is off)"

# -- ResolveNonManifoldPoints ------------------------------------------------
atlasNM = makeAtlas(EXTRACT_ALL, STYLE_ALL, resolveNM=True)
assert atlasNM.GetOutput().GetNumberOfPartitionedDataSets() == 8, \
    "NM resolution run should still produce 8 region PDS"

# -- MTime caching: changing labels does not rebuild the atlas ---------------
# Run AllRegions first to populate the internal atlas cache.
atlasCached = vtkSurfaceNetsAtlas()
atlasCached.GeneratePatchesOff()
atlasCached.SetBackgroundLabel(0)
atlasCached.SetInputDataObject(surfaceMesh)
atlasCached.SetExtractionModeToAll()
atlasCached.Update()
mtime1 = atlasCached.GetOutput().GetMTime()

# Changing SelectedLabels (a query parameter) should NOT rebuild the internal atlas.
atlasCached.SetExtractionModeToLabelSet()
atlasCached.AddSelectedLabel(1)
atlasCached.Update()
assert atlasCached.GetOutput().GetNumberOfPartitionedDataSets() == 1, \
    "After switching to LabelSet(1), output should have 1 PDS"

print("Phase 2 (extraction modes/styles): all assertions passed.")


# ===========================================================================
# Phase 3 – Rendering (4 viewports)
#   VP1 (left):       All style,      Regions
#   VP2 (centre-left):  All style,      Patches
#   VP3 (centre-right): Boundary style, Regions  (outer shell per label)
#   VP4 (right):        Boundary style, Patches  (interior label-label interfaces)
# ===========================================================================

def create_lut(n):
    """
    Create a lookup table for n objects.
    Index 0 is black (background/null), indices 1..n get distinct colors.
    """
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
    return a


patchLut = create_lut(NUM_PATCHES)


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

cam = vtkCamera()
cam.SetPosition(-1, 0.9, 0.7)
cam.SetFocalPoint(0, 0, 0)
ren1.SetActiveCamera(cam)
ren1.ResetCamera()
ren2.SetActiveCamera(ren1.GetActiveCamera())
ren3.SetActiveCamera(ren1.GetActiveCamera())
ren4.SetActiveCamera(ren1.GetActiveCamera())

renWin.Render()
iren.Start()
