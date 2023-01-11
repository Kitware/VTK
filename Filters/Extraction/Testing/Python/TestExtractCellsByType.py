#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import (
    VTK_HEXAHEDRON,
    VTK_LINE,
    VTK_QUAD,
    VTK_TETRA,
    VTK_TRIANGLE_STRIP,
    VTK_VERTEX,
    VTK_VOXEL,
    VTK_WEDGE,
    vtkQuadric,
)
from vtkmodules.vtkFiltersCore import (
    vtkAppendFilter,
    vtkAppendPolyData,
    vtkCellCenters,
    vtkExtractEdges,
    vtkStripper,
    vtkTriangleFilter,
)
from vtkmodules.vtkFiltersExtraction import vtkExtractCellsByType
from vtkmodules.vtkFiltersGeneral import (
    vtkClipVolume,
    vtkRandomAttributeGenerator,
)
from vtkmodules.vtkFiltersGeometry import vtkGeometryFilter
from vtkmodules.vtkImagingHybrid import vtkSampleFunction

# Test cell extraction by type by creating a hodgepodge of
# different datasets with different types. Then extracting
# cells and making sure the count is correct.

# Control test size
res = 20
error = 0

# Create an initial volume and extract different types of cells out of it.
#
# Quadric definition
quadric = vtkQuadric()
quadric.SetCoefficients([.5,1,.2,0,.1,0,0,.2,0,0])

sample = vtkSampleFunction()
sample.SetSampleDimensions((res+1),(res+1),(res+1))
sample.SetImplicitFunction(quadric)
sample.ComputeNormalsOn()
sample.Update()

# Extract cells - should be res*res*res voxels
extr = vtkExtractCellsByType()
extr.SetInputConnection(sample.GetOutputPort())
extr.AddCellType(VTK_VOXEL)
extr.Update()
print("Number of voxels: {0}".format(extr.GetOutput().GetNumberOfCells()))
if extr.GetOutput().GetNumberOfCells() != res*res*res:
    error = 1

# Extract hex cells - should be 0 hexes
extr.RemoveAllCellTypes()
extr.AddCellType(VTK_HEXAHEDRON)
extr.Update()
print("Number of hexes: {0}".format(extr.GetOutput().GetNumberOfCells()))
if extr.GetOutput().GetNumberOfCells() != 0:
    error = 1

# Generate tetrahedral mesh. The side effect of the clip filter
# is to produce tetrahedra.
clip = vtkClipVolume()
clip.SetInputConnection(sample.GetOutputPort())
clip.SetValue(-10.0)
clip.GenerateClippedOutputOff()
clip.Update()

# Create point and cell data arrays to verify whether they are properly copied
arrayGenerator = vtkRandomAttributeGenerator()
arrayGenerator.SetInputConnection(clip.GetOutputPort())
arrayGenerator.SetGenerateCellScalars(1)
arrayGenerator.SetGeneratePointScalars(1)
arrayGenerator.Update()

# Extract tetra cells as unstructured grid - should be 5*res*res*res tets
extr.SetInputConnection(arrayGenerator.GetOutputPort())
extr.AddCellType(VTK_TETRA)
extr.Update()
print("Number of tets: {0}".format(extr.GetOutput().GetNumberOfCells()))
if extr.GetOutput().GetNumberOfCells() != 5*res*res*res:
    error = 1

# Check number of points
if extr.GetOutput().GetNumberOfPoints() != 9261:
    print("Wrong number of points. Expected 9261 but got {0}.".format(extr.GetOutput().GetNumberOfPoints()))
    error = 1

# Check presence of cell and point data arrays
if extr.GetOutput().GetCellData().HasArray("RandomCellScalars") != 1:
    print("Missing cell array 'RandomCellScalars'.")
    error = 1

if extr.GetOutput().GetPointData().HasArray("RandomPointScalars") != 1:
    print("Missing cell array 'RandomPointScalars'.")
    error = 1

# Extract wedge cells as unstructured grid - should be 0 wedges
extr.RemoveCellType(VTK_TETRA)
extr.AddCellType(VTK_WEDGE)
extr.Update()
print("Number of wedges: {0}".format(extr.GetOutput().GetNumberOfCells()))
if extr.GetOutput().GetNumberOfCells() != 0:
    error = 1

# Now construct unstructured grid as conglomerate of tets and voxels. Should
# produce res*res*res + 5*res*res*res cells
appendU = vtkAppendFilter()
appendU.AddInputConnection(sample.GetOutputPort())
appendU.AddInputConnection(clip.GetOutputPort())

extr.SetInputConnection(appendU.GetOutputPort())
extr.AddAllCellTypes()
extr.Update()
print("Number of unstructured cells: {0}".format(extr.GetOutput().GetNumberOfCells()))
if extr.GetOutput().GetNumberOfCells() != 6*res*res*res:
    error = 1

extr.RemoveAllCellTypes()
extr.AddCellType(VTK_TETRA)
extr.Update()
print("\tNumber of tets: {0}".format(extr.GetOutput().GetNumberOfCells()))
if extr.GetOutput().GetNumberOfCells() != 5*res*res*res:
    error = 1

extr.RemoveAllCellTypes()
extr.AddCellType(VTK_VOXEL)
extr.Update()
print("\tNumber of voxels: {0}".format(extr.GetOutput().GetNumberOfCells()))
if extr.GetOutput().GetNumberOfCells() != res*res*res:
    error = 1

# Now construct polydata as a conglomerate of verts, lines, polys
centers = vtkCellCenters()
centers.SetInputConnection(sample.GetOutputPort())
centers.VertexCellsOn()

edges = vtkExtractEdges()
edges.SetInputConnection(sample.GetOutputPort())

polys = vtkGeometryFilter()
polys.SetInputConnection(sample.GetOutputPort())

appendP = vtkAppendPolyData()
appendP.AddInputConnection(centers.GetOutputPort())
appendP.AddInputConnection(edges.GetOutputPort())
appendP.AddInputConnection(polys.GetOutputPort())
appendP.Update()

extr.SetInputConnection(appendP.GetOutputPort())
extr.AddAllCellTypes()
extr.Update()
print("Number of polydata cells: {0}".format(extr.GetOutput().GetNumberOfCells()))
if extr.GetOutput().GetNumberOfCells() != (res*res*res) + 3*res*(res+1)*(res+1) + 6*res*res:
    error = 1

extr.SetInputConnection(appendP.GetOutputPort())
extr.RemoveAllCellTypes()
extr.AddCellType(VTK_VERTEX)
extr.Update()
print("\tNumber of verts: {0}".format(extr.GetOutput().GetNumberOfCells()))
if extr.GetOutput().GetNumberOfCells() != res*res*res:
    error = 1

extr.SetInputConnection(appendP.GetOutputPort())
extr.RemoveCellType(VTK_VERTEX)
extr.AddCellType(VTK_LINE)
extr.Update()
print("\tNumber of lines: {0}".format(extr.GetOutput().GetNumberOfCells()))
if extr.GetOutput().GetNumberOfCells() != 3*res*(res+1)*(res+1):
    error = 1

extr.SetInputConnection(appendP.GetOutputPort())
extr.RemoveCellType(VTK_LINE)
extr.AddCellType(VTK_QUAD)
extr.Update()
print("\tNumber of polys: {0}".format(extr.GetOutput().GetNumberOfCells()))
if extr.GetOutput().GetNumberOfCells() != 6*res*res:
    error = 1

# Finally triangle strips. The number of triangle strips may vary depending
# on how threaded execution of the geometry filter works. So we just look to
# make sure triangle strips are generated. Empirically, we know at least 99
# strips will be generated.
tris = vtkTriangleFilter()
tris.SetInputConnection(polys.GetOutputPort())

stripper = vtkStripper()
stripper.SetInputConnection(tris.GetOutputPort())

extr.SetInputConnection(stripper.GetOutputPort())
extr.RemoveCellType(VTK_LINE)
extr.AddCellType(VTK_TRIANGLE_STRIP)
extr.Update()
print("Number of triangle strips: {0}".format(extr.GetOutput().GetNumberOfCells()))
if extr.GetOutput().GetNumberOfCells() < 99:
    error = 1


# Return test results. If the assert is not true, then different results were
# generated by vtkStaticPointLocator and vtkPointLocator.
assert error == 0

# --- end of script --
