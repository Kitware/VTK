#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# VTK_DEPRECATED_IN_9_2_0
import warnings
warnings.filterwarnings('ignore', category=DeprecationWarning)

# Test and compare vtkGeometryFilter verus
# vtkDataSetSurfaceFilter.

# Control test resolution
res = 300
res = 50

# Generate hexes or tets
genHexes = 0

# Create a synthetic source, then convert to unstructured grid
vol = vtk.vtkImageData()
vol.SetDimensions(res,res,res)

sphere = vtk.vtkSphere()
sphere.SetRadius(10000)

grid = vtk.vtkExtractGeometry()
grid.SetInputData(vol)
grid.SetImplicitFunction(sphere)

# Alternative way to create tetra
tetras = vtk.vtkDataSetTriangleFilter()
tetras.SetInputData(vol)
if genHexes:
    grid.Update()
    print("Processing {0} hexes".format(grid.GetOutput().GetNumberOfCells()))
else:
    tetras.Update()
    print("Processing {0} tets".format(tetras.GetOutput().GetNumberOfCells()))

# Create a scalar field
ele = vtk.vtkSimpleElevationFilter()
if genHexes == 1:
    ele.SetInputConnection(grid.GetOutputPort())
else:
    ele.SetInputConnection(tetras.GetOutputPort())
ele.Update()

# Extract the surface with vtkGeometryFilter and time it. Use
# the fast extraction mode.
geomF = vtk.vtkGeometryFilter()
geomF.SetInputConnection(ele.GetOutputPort())
geomF.FastModeOn()
geomF.SetDegree(4)
geomF.MergingOn()

timer = vtk.vtkTimerLog()

timer.StartTimer()
geomF.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Geometry Filter: {0}".format(time))
print("\tNumber points: {0}".format(geomF.GetOutput().GetNumberOfPoints()))
print("\tNumber faces: {0}".format(geomF.GetOutput().GetNumberOfCells()))
print("")

# Extract the surface with vtkDataSetSurfaceFilter and time it
geomF2 = vtk.vtkDataSetSurfaceFilter()
geomF2.SetInputConnection(ele.GetOutputPort())

timer.StartTimer()
geomF2.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("DataSet Surface Filter: {0}".format(time))
print("\tNumber points: {0}".format(geomF2.GetOutput().GetNumberOfPoints()))
print("\tNumber faces: {0}".format(geomF2.GetOutput().GetNumberOfCells()))
print("")

# Show the result
mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(geomF.GetOutputPort())
mapper.SetScalarRange(0,float(res-1))

actor = vtk.vtkActor()
actor.SetMapper(mapper)

# Define graphics objects
ren1 = vtk.vtkRenderer()
ren1.SetBackground(0,0,0)
ren1.AddActor(actor)
ren1.GetActiveCamera().SetPosition(1,0,0)
ren1.ResetCamera()

renWin = vtk.vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

iren.Initialize()
iren.Start()
# --- end of script --
