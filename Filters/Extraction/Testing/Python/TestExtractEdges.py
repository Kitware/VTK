#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import vtkQuadric
from vtkmodules.vtkFiltersCore import (
    vtkExecutionTimer,
    vtkExtractEdges,
    vtkPointDataToCellData,
)
from vtkmodules.vtkImagingHybrid import vtkSampleFunction
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
res = 10

# Test edge extraction for arbitrary datasets
#
# Quadric definition
quadric = vtkQuadric()
quadric.SetCoefficients([.5,1,.2,0,.1,0,0,.2,0,0])
sample = vtkSampleFunction()
sample.SetSampleDimensions(res,res,res)
sample.SetImplicitFunction(quadric)
sample.ComputeNormalsOff()
sample.Update()

# Create some cell data
pd2cd = vtkPointDataToCellData()
pd2cd.SetInputConnection(sample.GetOutputPort())
pd2cd.PassPointDataOff()
pd2cd.Update()

# Now extract the edges
extract = vtkExtractEdges()
extract.SetInputConnection(pd2cd.GetOutputPort())
extract.UseAllPointsOn()

timer = vtkExecutionTimer()
timer.SetFilter(extract)
extract.Update()
ET = timer.GetElapsedWallClockTime()
print ("vtkExtractEdges:", ET)

extrMapper = vtkPolyDataMapper()
extrMapper.SetInputConnection(extract.GetOutputPort())
extrMapper.ScalarVisibilityOn()

extrActor = vtkActor()
extrActor.SetMapper(extrMapper)

# Define graphics objects
renWin = vtkRenderWindow()
renWin.SetSize(300,300)

ren1 = vtkRenderer()
ren1.SetBackground(0,0,0)

renWin.AddRenderer(ren1)

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren1.AddActor(extrActor)

ren1.ResetCamera()
renWin.Render()

iren.Initialize()
iren.Start()
# --- end of script --
