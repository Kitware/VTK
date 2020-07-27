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
edge = [57,58]
edges.InsertNextCell(2,edge)
edge = [58,59]
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
conn.ColorRegionsOn()
conn.Update()

tessMapper = vtk.vtkPolyDataMapper()
tessMapper.SetInputConnection(conn.GetOutputPort())
#tessMapper.SetInputConnection(cs.GetOutputPort())
tessMapper.ScalarVisibilityOn()
tessMapper.SetScalarModeToUseCellData()
tessMapper.SetScalarRange(0,conn.GetNumberOfExtractedRegions()-1)
print("Num cells: ",conn.GetOutput().GetNumberOfCells())
print("Num regions: ",conn.GetNumberOfExtractedRegions())

tessActor = vtk.vtkActor()
tessActor.SetMapper(tessMapper)
tessActor.GetProperty().SetColor(1,1,1)
tessActor.GetProperty().EdgeVisibilityOn()
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
