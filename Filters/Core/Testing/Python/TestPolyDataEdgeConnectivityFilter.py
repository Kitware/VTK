#!/usr/bin/env python
import sys
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Test the vtkPolyDataEdgeConnectivityFilter, in particular
# barrier edges and region growing.
# Create different points with variable spacing.
# Also a scalar field. Perform a combined segmentation
# based on barrier edges (defined by edge length) and
# scalar values.

# Construct a bounding array of points to improve the
# tessellation process.
plane = vtk.vtkPlaneSource()
plane.SetResolution(10,10)
plane.SetOrigin(-2,-2,0)
plane.SetPoint1(2,-2,0)
plane.SetPoint2(-2,2,0)

# Remove interior points
pedges = vtk.vtkFeatureEdges()
pedges.SetInputConnection(plane.GetOutputPort())
pedges.ExtractAllEdgeTypesOff()
pedges.BoundaryEdgesOn()
pedges.Update()

# Create some points in concentric circles.
disk = vtk.vtkDiskSource()
disk.SetInnerRadius(0.5)
disk.SetOuterRadius(1.0)
disk.SetRadialResolution(1)
disk.SetCircumferentialResolution(32)
disk.Update()

# Append plane points and disk points.
append = vtk.vtkAppendPolyData()
append.AddInputData(pedges.GetOutput())
append.AddInputData(disk.GetOutput())
append.Update()

# Tessellate
tess = vtk.vtkDelaunay2D()
tess.SetInputConnection(append.GetOutputPort())
tess.Update()

# Color via connected regions
conn = vtk.vtkPolyDataEdgeConnectivityFilter()
conn.SetInputConnection(tess.GetOutputPort());
conn.BarrierEdgesOn()
conn.SetBarrierEdgeLength(0.0,0.20)
conn.GrowLargeRegionsOn()
conn.SetExtractionModeToLargeRegions()
conn.SetLargeRegionThreshold(0.04)
conn.ColorRegionsOn()
conn.Update()

tessMapper = vtk.vtkPolyDataMapper()
tessMapper.SetInputConnection(conn.GetOutputPort())
tessMapper.ScalarVisibilityOn()
tessMapper.SetScalarModeToUseCellData()
tessMapper.SetScalarRange(0,conn.GetNumberOfExtractedRegions()-1)
tessMapper.SetScalarRange(0,5)
print("Num cells: ",conn.GetOutput().GetNumberOfCells())
print("Num regions: ",conn.GetNumberOfExtractedRegions())

tessActor = vtk.vtkActor()
tessActor.SetMapper(tessMapper)
tessActor.GetProperty().SetColor(1,1,1)
#tessActor.GetProperty().EdgeVisibilityOn()
tessActor.GetProperty().SetEdgeColor(0,0,0)

# Define graphics objects
ren1 = vtk.vtkRenderer()
ren1.SetBackground(0,0,0)
ren1.AddActor(tessActor)

renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

iren.Initialize()
iren.Start()
