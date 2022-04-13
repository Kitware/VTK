#!/usr/bin/env python
import vtk

# Test edge extraction with cell data, and with original
# point numbering preserved.
#res = 200
res = 10

# Test edge extraction for arbitrary datasets
#
# Quadric definition
quadric = vtk.vtkQuadric()
quadric.SetCoefficients([.5,1,.2,0,.1,0,0,.2,0,0])
sample = vtk.vtkSampleFunction()
sample.SetSampleDimensions(res,res,res)
sample.SetImplicitFunction(quadric)
sample.ComputeNormalsOff()
sample.Update()

# Create some cell data
pd2cd = vtk.vtkPointDataToCellData()
pd2cd.SetInputConnection(sample.GetOutputPort())
pd2cd.PassPointDataOff()
pd2cd.Update()

# Now extract the edges
extract = vtk.vtkExtractEdges()
extract.SetInputConnection(pd2cd.GetOutputPort())
extract.UseAllPointsOn()

timer = vtk.vtkExecutionTimer()
timer.SetFilter(extract)
extract.Update()
ET = timer.GetElapsedWallClockTime()
print ("vtkExtractEdges:", ET)

extrMapper = vtk.vtkPolyDataMapper()
extrMapper.SetInputConnection(extract.GetOutputPort())
extrMapper.ScalarVisibilityOn()

extrActor = vtk.vtkActor()
extrActor.SetMapper(extrMapper)

# Define graphics objects
renWin = vtk.vtkRenderWindow()
renWin.SetSize(300,300)

ren1 = vtk.vtkRenderer()
ren1.SetBackground(0,0,0)

renWin.AddRenderer(ren1)

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren1.AddActor(extrActor)

ren1.ResetCamera()
renWin.Render()

iren.Initialize()
iren.Start()
# --- end of script --
