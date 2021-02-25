#!/usr/bin/env python
import sys
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Test the vtkPolyDataEdgeConnectivityFilter, in particular
# barrier edges and region growing alongside cell scalar
# segmentation.

# Construct a plane of regularly spaced points.
plane = vtk.vtkPlaneSource()
plane.SetResolution(10,10)
plane.SetOrigin(0,0,0)
plane.SetPoint1(10,0,0)
plane.SetPoint2(0,10,0)

# Now color it.
ele = vtk.vtkSimpleElevationFilter()
ele.SetInputConnection(plane.GetOutputPort())
ele.SetVector(1,0,0)

# Need cell scalars
cs = vtk.vtkPointDataToCellData()
cs.SetInputConnection(ele.GetOutputPort())
cs.Update()

# Create barrier edges
edges = vtk.vtkCellArray()
edge = [68,69]
edges.InsertNextCell(2,edge)
edge = [69,70]
edges.InsertNextCell(2,edge)

bedges = vtk.vtkPolyData()
bedges.SetPoints(cs.GetOutput().GetPoints())
bedges.SetLines(edges)

# Color via connected regions
conn = vtk.vtkPolyDataEdgeConnectivityFilter()
conn.SetInputConnection(cs.GetOutputPort());
conn.SetSourceData(bedges);
conn.ScalarConnectivityOn()
conn.SetScalarRange(2,4)
conn.BarrierEdgesOn()
conn.GrowLargeRegionsOff()
conn.SetExtractionModeToLargestRegion()
conn.SetExtractionModeToAllRegions()
#conn.SetExtractionModeToLargeRegions()
#conn.SetLargeRegionThreshold(0.075)
conn.ColorRegionsOn()
conn.CellRegionAreasOn()
conn.Update()

tessMapper = vtk.vtkPolyDataMapper()
tessMapper.SetInputConnection(conn.GetOutputPort())
tessMapper.ScalarVisibilityOn()
tessMapper.SetScalarModeToUseCellData()
tessMapper.SetScalarRange(0,conn.GetNumberOfExtractedRegions()-1)
tessMapper.SetScalarRange(0,2) # Color just the top two regions
print("Num cells: ",conn.GetOutput().GetNumberOfCells())
print("Num regions: ",conn.GetNumberOfExtractedRegions())
print("Total area: ",conn.GetTotalArea())

tessActor = vtk.vtkActor()
tessActor.SetMapper(tessMapper)
tessActor.GetProperty().SetColor(1,1,1)
tessActor.GetProperty().EdgeVisibilityOn()
tessActor.GetProperty().SetEdgeColor(0,0,0)

# For debugging
planeMapper = vtk.vtkPolyDataMapper()
planeMapper.SetInputConnection(cs.GetOutputPort())
planeMapper.ScalarVisibilityOn()
planeMapper.SetScalarModeToUseCellData()
planeMapper.SetScalarRange(cs.GetOutput().GetCellData().GetScalars().GetRange())
planeActor = vtk.vtkActor()
planeActor.SetMapper(planeMapper)
#planeActor.GetProperty().SetRepresentationToWireframe()

# Define graphics objects
ren1 = vtk.vtkRenderer()
ren1.SetBackground(0,0,0)
ren1.AddActor(tessActor)
#ren1.AddActor(planeActor)

renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

iren.Initialize()
iren.Start()
