#!/usr/bin/env python
import sys
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Test the vtkPolyDataEdgeConnectivityFilter, in particular
# barrier edges combined with sorted regions.

# Construct a bounding array of points to improve the
# tessellation process.
plane = vtk.vtkPlaneSource()
plane.SetResolution(10,10)
plane.SetOrigin(-5,-5,0)
plane.SetPoint1(5,-5,0)
plane.SetPoint2(-5,5,0)

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

# Create some points in concentric circles.
disk2 = vtk.vtkDiskSource()
disk2.SetInnerRadius(2.0)
disk2.SetOuterRadius(3.5)
disk2.SetRadialResolution(1)
disk2.SetCircumferentialResolution(64)
disk2.Update()

# Append plane points and disk points.
append = vtk.vtkAppendPolyData()
append.AddInputData(pedges.GetOutput())
append.AddInputData(disk.GetOutput())
append.AddInputData(disk2.GetOutput())
append.Update()

# Tessellate
tess = vtk.vtkDelaunay2D()
tess.SetInputConnection(append.GetOutputPort())
tess.Update()

# Color via connected regions
conn = vtk.vtkPolyDataEdgeConnectivityFilter()
conn.SetInputConnection(tess.GetOutputPort());
conn.BarrierEdgesOn()
conn.GrowSmallRegionsOn()
conn.SetBarrierEdgeLength(0.0,0.35)
conn.SetExtractionModeToAllRegions()
conn.SetLargeRegionThreshold(0.25)
conn.ColorRegionsOn()
conn.CellRegionAreasOn()
conn.Update()

tessMapper = vtk.vtkPolyDataMapper()
tessMapper.SetInputConnection(conn.GetOutputPort())
tessMapper.ScalarVisibilityOn()
tessMapper.SetScalarModeToUseCellData()
tessMapper.SetScalarRange(0,2)
print("Num cells: ",conn.GetOutput().GetNumberOfCells())
print("Num regions: ",conn.GetNumberOfExtractedRegions())

tessActor = vtk.vtkActor()
tessActor.SetMapper(tessMapper)
tessActor.GetProperty().SetColor(1,1,1)
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
