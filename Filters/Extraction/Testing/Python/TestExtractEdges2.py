#!/usr/bin/env python
from vtkmodules.vtkFiltersCore import (
    vtkExecutionTimer,
    vtkExtractEdges,
    vtkSimpleElevationFilter,
)
from vtkmodules.vtkFiltersSources import vtkPlaneSource
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

# Test edge extraction with cell data, and with original
# point numbering preserved.
#res = 200
res = 50

# Test polydata edge extraction
#
# Create a plane
plane = vtkPlaneSource()
plane.SetResolution(res,res)
plane.SetOrigin(0,0,0)
plane.SetPoint1(1,0,0)
plane.SetPoint2(0,0,1)
plane.Update()

# Create some cell data using an elevation filter
ele = vtkSimpleElevationFilter()
ele.SetInputConnection(plane.GetOutputPort())
ele.Update()

# Now extract the edges
extract = vtkExtractEdges()
extract.SetInputConnection(ele.GetOutputPort())
extract.UseAllPointsOn()

timer = vtkExecutionTimer()
timer.SetFilter(extract)
extract.Update()
ET = timer.GetElapsedWallClockTime()
print ("vtkExtractEdges (polygonal):", ET)

extrMapper = vtkPolyDataMapper()
extrMapper.SetInputConnection(extract.GetOutputPort())
extrMapper.ScalarVisibilityOn()

extrActor = vtkActor()
extrActor.SetMapper(extrMapper)
extrActor.GetProperty().SetInterpolationToFlat()

# Define graphics objects
renWin = vtkRenderWindow()
renWin.SetSize(300,300)

ren1 = vtkRenderer()
ren1.SetBackground(0,0,0)

renWin.AddRenderer(ren1)

iren = vtkRenderWindowInteractor()
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
