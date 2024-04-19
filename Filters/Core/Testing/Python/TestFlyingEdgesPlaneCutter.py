#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import (
    vtkPlane,
    vtkSphere,
)
from vtkmodules.vtkFiltersCore import (
    vtkCutter,
    vtkExecutionTimer,
    vtkFlyingEdgesPlaneCutter,
)
from vtkmodules.vtkFiltersModeling import vtkOutlineFilter
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
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

useFECutter = 1
res = 100

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Create a synthetic source: sample a sphere across a volume
sphere = vtkSphere()
sphere.SetCenter( 0.0,0.0,0.0)
sphere.SetRadius(0.25)

sample = vtkSampleFunction()
sample.SetImplicitFunction(sphere)
sample.SetModelBounds(-0.5,0.5, -0.5,0.5, -0.5,0.5)
sample.SetSampleDimensions(res,res,res)
sample.Update()

# The cut plane
plane = vtkPlane()
plane.SetOrigin(0,0,0)
plane.SetNormal(1,1,1)

if useFECutter:
    cut = vtkFlyingEdgesPlaneCutter()
    cut.SetInputConnection(sample.GetOutputPort())
    cut.SetPlane(plane)
    cut.ComputeNormalsOff() #make it equivalent to vtkCutter
else:
    # Compare against previous method
    cut = vtkCutter()
    cut.SetInputConnection(sample.GetOutputPort())
    cut.SetCutFunction(plane)
    cut.SetValue(0,0.0)

# Time the execution of the filter w/out scalar tree
CG_timer = vtkExecutionTimer()
CG_timer.SetFilter(cut)
cut.Update()
CG = CG_timer.GetElapsedWallClockTime()
print ("Cut volume:", CG)

cutMapper = vtkPolyDataMapper()
cutMapper.SetInputConnection(cut.GetOutputPort())

cutActor = vtkActor()
cutActor.SetMapper(cutMapper)
cutActor.GetProperty().SetColor(1,1,1)
cutActor.GetProperty().SetOpacity(1)

outline = vtkOutlineFilter()
outline.SetInputConnection(sample.GetOutputPort())

outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineProp = outlineActor.GetProperty()

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(outlineActor)
ren1.AddActor(cutActor)
ren1.SetBackground(0,0,0)
renWin.SetSize(400,400)
ren1.ResetCamera()
iren.Initialize()

renWin.Render()
# --- end of script --
#iren.Start()
