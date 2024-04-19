#!/usr/bin/env python
from vtkmodules.vtkFiltersCore import (
    vtkPolyDataNormals,
    vtkTensorGlyph,
)
from vtkmodules.vtkFiltersGeometry import vtkImageDataGeometryFilter
from vtkmodules.vtkFiltersModeling import vtkOutlineFilter
from vtkmodules.vtkFiltersSources import (
    vtkConeSource,
    vtkSphereSource,
)
from vtkmodules.vtkImagingHybrid import vtkPointLoad
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkCamera,
    vtkLogLookupTable,
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

# create tensor ellipsoids
# Create the RenderWindow, Renderer and interactive renderer
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
#
# Create tensor ellipsoids
#
# generate tensors
ptLoad = vtkPointLoad()
ptLoad.SetLoadValue(100.0)
ptLoad.SetSampleDimensions(6,6,6)
ptLoad.ComputeEffectiveStressOn()
ptLoad.SetModelBounds(-10,10,-10,10,-10,10)
# extract plane of data
plane = vtkImageDataGeometryFilter()
plane.SetInputConnection(ptLoad.GetOutputPort())
plane.SetExtent(2,2,0,99,0,99)
# Generate ellipsoids
sphere = vtkSphereSource()
sphere.SetThetaResolution(8)
sphere.SetPhiResolution(8)
ellipsoids = vtkTensorGlyph()
ellipsoids.SetInputConnection(ptLoad.GetOutputPort())
ellipsoids.SetSourceConnection(sphere.GetOutputPort())
ellipsoids.SetScaleFactor(10)
ellipsoids.ClampScalingOn()
ellipNormals = vtkPolyDataNormals()
ellipNormals.SetInputConnection(ellipsoids.GetOutputPort())
# Map contour
lut = vtkLogLookupTable()
lut.SetHueRange(.6667,0.0)
ellipMapper = vtkPolyDataMapper()
ellipMapper.SetInputConnection(ellipNormals.GetOutputPort())
ellipMapper.SetLookupTable(lut)
plane.Update()
#force update for scalar range
ellipMapper.SetScalarRange(plane.GetOutput().GetScalarRange())
ellipActor = vtkActor()
ellipActor.SetMapper(ellipMapper)
#
# Create outline around data
#
outline = vtkOutlineFilter()
outline.SetInputConnection(ptLoad.GetOutputPort())
outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(0,0,0)
#
# Create cone indicating application of load
#
coneSrc = vtkConeSource()
coneSrc.SetRadius(.5)
coneSrc.SetHeight(2)
coneMap = vtkPolyDataMapper()
coneMap.SetInputConnection(coneSrc.GetOutputPort())
coneActor = vtkActor()
coneActor.SetMapper(coneMap)
coneActor.SetPosition(0,0,11)
coneActor.RotateY(90)
coneActor.GetProperty().SetColor(1,0,0)
camera = vtkCamera()
camera.SetFocalPoint(0.113766,-1.13665,-1.01919)
camera.SetPosition(-29.4886,-63.1488,26.5807)
camera.SetViewAngle(24.4617)
camera.SetViewUp(0.17138,0.331163,0.927879)
camera.SetClippingRange(1,100)
ren1.AddActor(ellipActor)
ren1.AddActor(outlineActor)
ren1.AddActor(coneActor)
ren1.SetBackground(1.0,1.0,1.0)
ren1.SetActiveCamera(camera)
renWin.SetSize(400,400)
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
