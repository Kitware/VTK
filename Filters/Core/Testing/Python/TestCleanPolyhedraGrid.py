#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot

# Pipeline creation follows
#
pl3d = vtk.vtkMultiBlockPLOT3DReader()
pl3d.SetXYZFileName( vtkGetDataRoot() + '/Data/combxyz.bin' )
pl3d.SetQFileName( vtkGetDataRoot() + '/Data/combq.bin' )
pl3d.SetScalarFunctionNumber( 100 )
pl3d.SetVectorFunctionNumber( 202 )
pl3d.Update()
pl3d_output = pl3d.GetOutput().GetBlock(0)

# Convert to an unstructured grid
extSphere = vtk.vtkSphere()
extSphere.SetCenter(0,0,0)
extSphere.SetRadius(1000)
ugrid = vtk.vtkExtractGeometry()
ugrid.SetInputData(pl3d_output)
ugrid.SetImplicitFunction(extSphere)
ugrid.Update()

# The results of this processing.
sample = ugrid.GetOutput()

# Convert mesh to polyhedra
convert0 = vtk.vtkConvertToPolyhedra()
convert0.SetInputData(sample)
convert0.Update()

# Clean the data with zero tolerance
clean0 = vtk.vtkStaticCleanUnstructuredGrid()
clean0.SetInputConnection(convert0.GetOutputPort())
clean0.ToleranceIsAbsoluteOn()
clean0.SetTolerance(0.0)
clean0.RemoveUnusedPointsOff()

# Time execution
timer = vtk.vtkTimerLog()
timer.StartTimer()
clean0.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Cleaning an unstructured grid with: {0} points and {1} cells".format(sample.GetNumberOfPoints(),sample.GetNumberOfCells()))
print("Time to clean grid (zero tolerance): {0}".format(time))
print("\tRemaining points: {0}".format(clean0.GetOutput().GetNumberOfPoints()))
print("\tRemaining cells: {0}".format(clean0.GetOutput().GetNumberOfCells()))

# Clean the data with non-zero tolerance
clean1 = vtk.vtkStaticCleanUnstructuredGrid()
clean1.SetInputConnection(convert0.GetOutputPort())
clean1.ToleranceIsAbsoluteOff()
clean1.SetTolerance(0.00001)
clean1.RemoveUnusedPointsOn()

# Time execution
timer.StartTimer()
clean1.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Time to clean grid (non-zero tolerance): {0}".format(time))
print("\tRemaining points: {0}".format(clean1.GetOutput().GetNumberOfPoints()))
print("\tRemaining cells: {0}".format(clean1.GetOutput().GetNumberOfCells()))

# Now shrink the data and clean with zero and non-zero tolerance
shrink0 = vtk.vtkShrinkFilter()
shrink0.SetInputData(sample)
shrink0.SetShrinkFactor(1.0)
shrink0.Update()

# Convert mesh to polyhedra
convert0.SetInputConnection(shrink0.GetOutputPort())

# This is with a zero tolerance
clean0.SetInputConnection(convert0.GetOutputPort())
clean0.AveragePointDataOn()

timer.StartTimer()
clean0.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("\nCleaning shrunk data with: {0} points and {1} cells".format(shrink0.GetOutput().GetNumberOfPoints(),shrink0.GetOutput().GetNumberOfCells()))
print("Time to clean grid (zero tolerance): {0}".format(time))
print("\tRemaining points: {0}".format(clean0.GetOutput().GetNumberOfPoints()))
print("\tRemaining cells: {0}".format(clean0.GetOutput().GetNumberOfCells()))

shrink1 = vtk.vtkShrinkFilter()
shrink1.SetInputData(sample)
shrink1.SetShrinkFactor(0.999)
shrink1.Update()

# Convert mesh to polyhedra
convert1 = vtk.vtkConvertToPolyhedra()
convert1.SetInputConnection(shrink1.GetOutputPort())

clean1.SetInputConnection(convert1.GetOutputPort())
clean1.ToleranceIsAbsoluteOn()
clean1.SetAbsoluteTolerance(0.01)
clean1.ProduceMergeMapOn()
clean1.AveragePointDataOff()

timer.StartTimer()
clean1.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Time to clean grid (non-zero tolerance): {0}".format(time))
print("\tRemaining points: {0}".format(clean1.GetOutput().GetNumberOfPoints()))
print("\tRemaining cells: {0}".format(clean1.GetOutput().GetNumberOfCells()))

# Graphics stuff
# Create the RenderWindow, Renderers and both Actors
ren0 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren0)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

mapper1 = vtk.vtkDataSetMapper()
mapper1.SetInputConnection(clean1.GetOutputPort())
mapper1.SetScalarRange(sample.GetPointData().GetScalars().GetRange())

actor1 = vtk.vtkActor()
actor1.SetMapper(mapper1)
actor1.GetProperty().SetInterpolationToFlat()

# Add the actors to the renderer, set the background and size
ren0.AddActor(actor1)

ren0.SetBackground(0,0,0)
renWin.SetSize(300,300)
camera = ren0.GetActiveCamera()
camera.SetPosition(1,1,1)
ren0.ResetCamera()
iren.Initialize()

renWin.Render()
iren.Start()
# --- end of script --
