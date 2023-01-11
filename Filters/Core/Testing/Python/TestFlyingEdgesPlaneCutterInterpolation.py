#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import (
    vtkCylinder,
    vtkDataObject,
    vtkPlane,
    vtkSphere,
)
from vtkmodules.vtkFiltersCore import (
    vtkExecutionTimer,
    vtkFlyingEdgesPlaneCutter,
    vtkHedgeHog,
    vtkMaskPoints,
)
from vtkmodules.vtkFiltersGeneral import vtkSampleImplicitFunctionFilter
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
sample.ComputeNormalsOff()
sample.Update()

# Now create some new attributes to interpolate
cyl = vtkCylinder()
cyl.SetRadius(0.1)
cyl.SetAxis(1,1,1)

attr = vtkSampleImplicitFunctionFilter()
attr.SetInputConnection(sample.GetOutputPort())
attr.SetImplicitFunction(cyl)
attr.ComputeGradientsOn()
attr.Update()

# The cut plane
plane = vtkPlane()
plane.SetOrigin(-.2,-.2,-.2)
plane.SetNormal(1,1,1)

# Perform the cutting on named scalars
cut = vtkFlyingEdgesPlaneCutter()
cut.SetInputConnection(attr.GetOutputPort())
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

cutMapper = vtkPolyDataMapper()
cutMapper.SetInputConnection(cut.GetOutputPort())
cutMapper.SetScalarModeToUsePointFieldData()
cutMapper.SelectColorArray("Implicit scalars")

cutActor = vtkActor()
cutActor.SetMapper(cutMapper)
cutActor.GetProperty().SetColor(1,1,1)
cutActor.GetProperty().SetOpacity(1)

# Look at the vectors
ranPts = vtkMaskPoints()
ranPts.SetOnRatio(25)
ranPts.SetInputConnection(cut.GetOutputPort())

hhog = vtkHedgeHog()
hhog.SetInputConnection(ranPts.GetOutputPort())
hhog.SetVectorModeToUseVector()
hhog.SetScaleFactor(0.05)

hhogMapper = vtkPolyDataMapper()
hhogMapper.SetInputConnection(hhog.GetOutputPort())

hhogActor = vtkActor()
hhogActor.SetMapper(hhogMapper)
hhogActor.GetProperty().SetColor(1,1,1)
hhogActor.GetProperty().SetOpacity(1)

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
ren1.AddActor(outlineActor)
ren1.AddActor(cutActor)
ren1.AddActor(hhogActor)
ren1.SetBackground(0,0,0)
renWin.SetSize(400,400)
ren1.ResetCamera()
iren.Initialize()

renWin.Render()
# --- end of script --
