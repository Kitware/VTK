#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import vtkPlane
from vtkmodules.vtkCommonMath import vtkMatrix4x4
from vtkmodules.vtkFiltersCore import (
    vtkFlyingEdges2D,
    vtkFlyingEdges3D,
    vtkTubeFilter,
)
from vtkmodules.vtkFiltersModeling import vtkOutlineFilter
from vtkmodules.vtkImagingCore import (
    vtkImageReslice,
    vtkRTAnalyticSource,
)
from vtkmodules.vtkImagingHybrid import vtkSampleFunction
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkCamera,
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

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
ren2 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
ren1.SetViewport(0,0,0.5,1)
ren2.SetViewport(0.5,0,1,1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Test the negative extents
source = vtkRTAnalyticSource()

iso = vtkFlyingEdges3D()
iso.SetInputConnection(source.GetOutputPort())
iso.SetValue(0,150)

isoMapper = vtkPolyDataMapper()
isoMapper.SetInputConnection(iso.GetOutputPort())
isoMapper.ScalarVisibilityOff()

isoActor = vtkActor()
isoActor.SetMapper(isoMapper)
isoActor.GetProperty().SetColor(1,1,1)
isoActor.GetProperty().SetOpacity(1)

outline = vtkOutlineFilter()
outline.SetInputConnection(source.GetOutputPort())

outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineProp = outlineActor.GetProperty()

# Test the non-x-edge-intersecting contours and cuts
res = 100
plane = vtkPlane()
plane.SetOrigin(0,0,0)
plane.SetNormal(0,1,0)

sample = vtkSampleFunction()
sample.SetImplicitFunction(plane)
sample.SetModelBounds(-10,10, -10,10, -10,10)
sample.SetSampleDimensions(res,res,res)
sample.SetOutputScalarTypeToFloat()
sample.Update()

iso2 = vtkFlyingEdges3D()
iso2.SetInputConnection(sample.GetOutputPort())
iso2.SetValue(0,0.0)

isoMapper2 = vtkPolyDataMapper()
isoMapper2.SetInputConnection(iso2.GetOutputPort())
isoMapper2.ScalarVisibilityOff()

isoActor2 = vtkActor()
isoActor2.SetMapper(isoMapper2)
isoActor2.GetProperty().SetColor(1,1,1)
isoActor2.GetProperty().SetOpacity(1)

outline2 = vtkOutlineFilter()
outline2.SetInputConnection(sample.GetOutputPort())

outlineMapper2 = vtkPolyDataMapper()
outlineMapper2.SetInputConnection(outline2.GetOutputPort())

outlineActor2 = vtkActor()
outlineActor2.SetMapper(outlineMapper)

# Extract a slice in the desired orientation
center = [0.0, 0.0, 0.0]
axial = vtkMatrix4x4()
axial.DeepCopy((1, 0, 0, center[0],
                0, 1, 0, center[1],
                0, 0, 1, center[2],
                0, 0, 0, 1))

reslice = vtkImageReslice()
reslice.SetInputConnection(sample.GetOutputPort())
reslice.SetOutputDimensionality(2)
reslice.SetResliceAxes(axial)
reslice.SetInterpolationModeToLinear()

iso3 = vtkFlyingEdges2D()
iso3.SetInputConnection(reslice.GetOutputPort())
iso3.SetValue(0,0.0)

tube = vtkTubeFilter()
tube.SetInputConnection(iso3.GetOutputPort())
tube.SetRadius(0.25)

mapper3 = vtkPolyDataMapper()
mapper3.SetInputConnection(tube.GetOutputPort())

actor3 = vtkActor()
actor3.SetMapper(mapper3)


# Add the actors to the renderer, set the background and size
#
renWin.SetSize(600,300)
ren1.AddActor(outlineActor)
ren1.AddActor(isoActor)
ren1.SetBackground(0,0,0)
ren1.ResetCamera()

cam = vtkCamera()
cam.SetPosition(0,1,0)
cam.SetFocalPoint(0,0,0)
cam.SetViewUp(0,0,1)
ren2.SetActiveCamera(cam)
ren2.AddActor(outlineActor2)
ren2.AddActor(isoActor2)
ren2.AddActor(actor3)
ren2.SetBackground(0,0,0)
ren2.ResetCamera()

iren.Initialize()

renWin.Render()
iren.Start()
# --- end of script --
