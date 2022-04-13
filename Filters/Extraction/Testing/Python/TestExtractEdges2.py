#!/usr/bin/env python
import vtk

# Test edge extraction with cell data, and with original
# point numbering preserved.
#res = 200
res = 50

# Test polydata edge extraction
#
# Create a plane
plane = vtk.vtkPlaneSource()
plane.SetResolution(res,res)
plane.SetOrigin(0,0,0)
plane.SetPoint1(1,0,0)
plane.SetPoint2(0,0,1)
plane.Update()

# Create some cell data using an elevation filter
ele = vtk.vtkSimpleElevationFilter()
ele.SetInputConnection(plane.GetOutputPort())
ele.Update()

# Now extract the edges
extract = vtk.vtkExtractEdges()
extract.SetInputConnection(ele.GetOutputPort())
extract.UseAllPointsOn()

timer = vtk.vtkExecutionTimer()
timer.SetFilter(extract)
extract.Update()
ET = timer.GetElapsedWallClockTime()
print ("vtkExtractEdges (polygonal):", ET)

extrMapper = vtk.vtkPolyDataMapper()
extrMapper.SetInputConnection(extract.GetOutputPort())
extrMapper.ScalarVisibilityOn()

extrActor = vtk.vtkActor()
extrActor.SetMapper(extrMapper)
extrActor.GetProperty().SetInterpolationToFlat()

# Define graphics objects
renWin = vtk.vtkRenderWindow()
renWin.SetSize(300,300)

ren1 = vtk.vtkRenderer()
ren1.SetBackground(0,0,0)

renWin.AddRenderer(ren1)

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren1.AddActor(extrActor)

ren1.GetActiveCamera().SetFocalPoint(0.5,0,0.5)
ren1.GetActiveCamera().SetPosition(0.5,1,0.5)
ren1.GetActiveCamera().SetViewUp(0,0,1)
ren1.ResetCamera()
renWin.Render()

iren.Initialize()
iren.Start()
# --- end of script --
