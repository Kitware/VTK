#!/usr/bin/env python
import sys
from vtkmodules.vtkFiltersCore import (
    vtkAppendPolyData,
    vtkDelaunay2D,
    vtkFeatureEdges,
    vtkPolyDataEdgeConnectivityFilter,
)
from vtkmodules.vtkFiltersSources import (
    vtkDiskSource,
    vtkPlaneSource,
)
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Test the vtkPolyDataEdgeConnectivityFilter, in particular
# barrier edges and region growing.
# Create different points with variable spacing.
# Also a scalar field. Perform a combined segmentation
# based on barrier edges (defined by edge length) and
# scalar values.

# Construct a bounding array of points to improve the
# tessellation process.
plane = vtkPlaneSource()
plane.SetResolution(10,10)
plane.SetOrigin(-2,-2,0)
plane.SetPoint1(2,-2,0)
plane.SetPoint2(-2,2,0)

# Remove interior points
pedges = vtkFeatureEdges()
pedges.SetInputConnection(plane.GetOutputPort())
pedges.ExtractAllEdgeTypesOff()
pedges.BoundaryEdgesOn()
pedges.Update()

# Create some points in concentric circles.
disk = vtkDiskSource()
disk.SetInnerRadius(0.5)
disk.SetOuterRadius(1.0)
disk.SetRadialResolution(1)
disk.SetCircumferentialResolution(32)
disk.Update()

# Append plane points and disk points.
append = vtkAppendPolyData()
append.AddInputData(pedges.GetOutput())
append.AddInputData(disk.GetOutput())
append.Update()

# Tessellate
tess = vtkDelaunay2D()
tess.SetInputConnection(append.GetOutputPort())
tess.Update()

# Color via connected regions
conn = vtkPolyDataEdgeConnectivityFilter()
conn.SetInputConnection(tess.GetOutputPort());
conn.BarrierEdgesOn()
conn.SetBarrierEdgeLength(0.0,0.20)
conn.GrowSmallRegionsOn()
conn.SetExtractionModeToAllRegions()
conn.SetLargeRegionThreshold(0.25)
conn.ColorRegionsOn()
conn.Update()

tessMapper = vtkPolyDataMapper()
tessMapper.SetInputConnection(conn.GetOutputPort())
tessMapper.ScalarVisibilityOn()
tessMapper.SetScalarModeToUseCellData()
tessMapper.SetScalarRange(0,conn.GetNumberOfExtractedRegions()-1)
tessMapper.SetScalarRange(0,5)
print("Num cells: ",conn.GetOutput().GetNumberOfCells())
print("Num regions: ",conn.GetNumberOfExtractedRegions())

tessActor = vtkActor()
tessActor.SetMapper(tessMapper)
tessActor.GetProperty().SetColor(1,1,1)
#tessActor.GetProperty().EdgeVisibilityOn()
tessActor.GetProperty().SetEdgeColor(0,0,0)

# Define graphics objects
ren1 = vtkRenderer()
ren1.SetBackground(0,0,0)
ren1.AddActor(tessActor)

renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

iren.Initialize()
iren.Start()
