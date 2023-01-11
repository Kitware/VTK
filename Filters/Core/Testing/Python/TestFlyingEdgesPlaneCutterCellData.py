#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import (
    vtkDataObject,
    vtkPlane,
    vtkSphere,
)
from vtkmodules.vtkFiltersCore import (
    vtkExecutionTimer,
    vtkFlyingEdgesPlaneCutter,
    vtkPointDataToCellData,
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

# Control size of test
res = 10

# Create the RenderWindow, Renderer and both Actors
#
ren0 = vtkRenderer()
ren0.SetViewport(0,0,0.5,1)
ren1 = vtkRenderer()
ren1.SetViewport(0.5,0,1,1)
renWin = vtkRenderWindow()
renWin.AddRenderer(ren0)
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
sample.ComputeNormalsOff()
sample.Update()

# Create some cell data
pd2cd = vtkPointDataToCellData()
pd2cd.SetInputConnection(sample.GetOutputPort())
pd2cd.PassPointDataOn()
pd2cd.Update()

# The cut plane
plane = vtkPlane()
plane.SetOrigin(-.2,-.2,-.2)
plane.SetNormal(1,1,1)

# Perform the cutting on named scalars
cut = vtkFlyingEdgesPlaneCutter()
cut.SetInputConnection(pd2cd.GetOutputPort())
cut.SetInputArrayToProcess(0, 0, 0, vtkDataObject.FIELD_ASSOCIATION_POINTS, "scalars")
cut.SetPlane(plane)
cut.ComputeNormalsOff()
cut.InterpolateAttributesOn()

# Time the execution of the filter
timer = vtkExecutionTimer()
timer.SetFilter(cut)
cut.Update()
CG = timer.GetElapsedWallClockTime()
print ("Cut and interpolate volume:", CG)

# Display the point data
cutMapper = vtkPolyDataMapper()
cutMapper.SetInputConnection(cut.GetOutputPort())

cutActor = vtkActor()
cutActor.SetMapper(cutMapper)

# Display the cell data
cellCutMapper = vtkPolyDataMapper()
cellCutMapper.SetInputConnection(cut.GetOutputPort())
cellCutMapper.SetScalarModeToUseCellData()

cellCutActor = vtkActor()
cellCutActor.SetMapper(cellCutMapper)

# Outline
outline = vtkOutlineFilter()
outline.SetInputConnection(sample.GetOutputPort())

outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineProp = outlineActor.GetProperty()

# Add the actors to the renderer, set the background and size
#
ren0.AddActor(outlineActor)
ren0.AddActor(cutActor)
ren0.SetBackground(0,0,0)
ren1.AddActor(outlineActor)
ren1.AddActor(cellCutActor)
ren1.SetBackground(0,0,0)

renWin.SetSize(600,300)
ren1.ResetCamera()
iren.Initialize()

renWin.Render()
iren.Start()
# --- end of script --
