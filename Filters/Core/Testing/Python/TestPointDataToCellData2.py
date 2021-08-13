#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Control resolution of the test
res = 300
res = 50

# Generate hexes or tets
genHexes = 1

# Use categorical data
catData = 0

# Create a synthetic source with point data
vol = vtk.vtkImageData()
vol.SetDimensions(res,res,res)

ele = vtk.vtkSimpleElevationFilter()
ele.SetInputData(vol)
ele.Update()
print("Processing {0} voxels".format(ele.GetOutput().GetNumberOfCells()))

# Convert point data to cell data
pd2cd = vtk.vtkPointDataToCellData()
pd2cd.SetInputConnection(ele.GetOutputPort())
pd2cd.SetCategoricalData(catData)

# For timing the various tests
timer = vtk.vtkTimerLog()
timer.StartTimer()
pd2cd.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Point Data To Cell Data (Structured): {0}".format(time))

# Convert to unstructured grid using threshold hack (side effect is
# conversion to vtkUnstructuredGrid)
extract = vtk.vtkThreshold()
extract.SetInputConnection(ele.GetOutputPort())
extract.SetThresholdFunction(vtk.vtkThreshold.THRESHOLD_BETWEEN)
extract.SetLowerThreshold(-10000.0)
extract.SetUpperThreshold(10000.0)
extract.Update()

# Create tests
tetras = vtk.vtkDataSetTriangleFilter()
tetras.SetInputConnection(extract.GetOutputPort())

if genHexes:
    output = extract.GetOutput()
    print("Processing {0} hexes".format(extract.GetOutput().GetNumberOfCells()))
else:
    tetras.Update()
    output = tetras.GetOutput()
    print("Processing {0} tets".format(tetras.GetOutput().GetNumberOfCells()))

# Now process the data
pd2cd2 = vtk.vtkPointDataToCellData()
pd2cd2.SetInputData(output)
pd2cd2.SetCategoricalData(catData)

timer.StartTimer()
pd2cd2.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Point Data To Cell Data: (Unstructured) {0}".format(time))

# Let's see what it looks like
scalarRange = pd2cd.GetOutput().GetCellData().GetScalars().GetRange()

geom = vtk.vtkGeometryFilter()
geom.SetInputConnection(pd2cd.GetOutputPort())

mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(geom.GetOutputPort())
mapper.SetScalarRange(scalarRange)

actor = vtk.vtkActor()
actor.SetMapper(mapper)

geom2 = vtk.vtkGeometryFilter()
geom2.SetInputConnection(pd2cd2.GetOutputPort())

mapper2 = vtk.vtkPolyDataMapper()
mapper2.SetInputConnection(geom2.GetOutputPort())
mapper2.SetScalarRange(scalarRange)

actor2 = vtk.vtkActor()
actor2.SetMapper(mapper2)

# Create the RenderWindow, Renderers and both Actors
ren0 = vtk.vtkRenderer()
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren0)
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren0.SetBackground(0,0,0)
ren0.AddActor(actor)
ren0.SetViewport(0,0,0.5,1);

ren1.SetBackground(0,0,0)
ren1.AddActor(actor)
ren1.SetViewport(0.5,0,1,1);

renWin.SetSize(600,300)

ren0.GetActiveCamera().SetPosition(1,0,0)
ren0.GetActiveCamera().SetFocalPoint(0,0,0)
ren0.ResetCamera()
ren0.GetActiveCamera().Zoom(1.25)
ren1.SetActiveCamera(ren0.GetActiveCamera())
iren.Initialize()

renWin.Render()
iren.Start()
# --- end of script --
