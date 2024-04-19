#!/usr/bin/env python
# -*- coding: utf-8 -*-

from vtkmodules.vtkCommonCore import vtkLookupTable
from vtkmodules.vtkCommonDataModel import (
    vtkImageData,
    vtkPlane,
)
from vtkmodules.vtkFiltersCore import (
    vtkCutter,
    vtkGlyph3D,
    vtkProbeFilter,
    vtkTensorGlyph,
)
from vtkmodules.vtkFiltersExtraction import vtkExtractTensorComponents
from vtkmodules.vtkFiltersModeling import (
    vtkOutlineFilter,
    vtkPolyDataPointSampler,
)
from vtkmodules.vtkFiltersPoints import vtkPointSmoothingFilter
from vtkmodules.vtkFiltersSources import vtkSphereSource
from vtkmodules.vtkImagingHybrid import vtkPointLoad
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
import vtkmodules.test.Testing
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Control test resolution
res = 30

# Controls the plane normal and view plane normal. Slightly
# off plane better tests the MotionConstraintToPlane.
normal = [0.1,0.1,1]

# Generate a sizing field. Use a synthetic volume with stress
# tensors.
ptLoad = vtkPointLoad()
ptLoad.SetModelBounds(-0.5,0.5, -0.5,0.5, -0.5,0.5)
ptLoad.SetSampleDimensions(res,res,res)
ptLoad.Update()

sizeField = ptLoad.GetOutput()
bounds = sizeField.GetBounds()
length = sizeField.GetLength()
center = [(bounds[1]+bounds[0])/2.0, (bounds[3]+bounds[2])/2.0, (bounds[5]+bounds[4])/2.0]

# Cut the data (which has tensors) with a plane
plane = vtkPlane()
plane.SetOrigin(center)
plane.SetNormal(normal)

# Create a single voxel the same size as the volume.
vox = vtkImageData()
vox.SetDimensions(2,2,2)
vox.SetOrigin(-0.5,-0.5,-0.5)
vox.SetSpacing(1,1,1)

# Cut the voxel to produce a polygon
cut = vtkCutter()
cut.SetInputData(vox)
cut.SetCutFunction(plane)
cut.Update()

# Now create points on the polygon
sampler = vtkPolyDataPointSampler()
sampler.SetInputConnection(cut.GetOutputPort())
sampler.SetDistance(0.0175)
sampler.SetPointGenerationModeToRandom()
sampler.GenerateVertexPointsOff()
sampler.GenerateEdgePointsOff()
sampler.GenerateInteriorPointsOn()
sampler.Update()

# Use these points to probe for tensor data
probe = vtkProbeFilter()
probe.SetInputConnection(sampler.GetOutputPort())
probe.SetSourceConnection(ptLoad.GetOutputPort())
probe.Update()

# Extract some tensor information
textract = vtkExtractTensorComponents()
textract.SetInputConnection(probe.GetOutputPort())
textract.ExtractScalarsOn()
textract.ScalarIsNonNegativeDeterminant()
textract.PassTensorsToOutputOn()
textract.Update()

# Now smooth/pack the points in a variety of ways.
# We'll glyph with a transformed sphere
sph = vtkSphereSource()
sph.SetRadius(0.5)
sph.SetCenter(0.0, 0.0, 0.0)
sph.SetThetaResolution(16)
sph.SetPhiResolution(8)
sph.Update()

# First show the points unsmoothed
smooth0 = vtkPointSmoothingFilter()
smooth0.SetInputConnection(textract.GetOutputPort())
smooth0.SetNumberOfIterations(0) #sends input to output
smooth0.SetSmoothingModeToDefault()
smooth0.Update()

glyph0 = vtkGlyph3D()
glyph0.SetInputConnection(smooth0.GetOutputPort())
glyph0.SetSourceConnection(sph.GetOutputPort())
glyph0.SetScaleModeToDataScalingOff()
glyph0.SetScaleFactor(0.025)

gMapper0 = vtkPolyDataMapper()
gMapper0.SetInputConnection(glyph0.GetOutputPort())
gMapper0.ScalarVisibilityOff()

gActor0 = vtkActor()
gActor0.SetMapper(gMapper0)
gActor0.GetProperty().SetColor(1,1,1)
gActor0.GetProperty().SetOpacity(1)

# Now the geometric behavior
smooth1 = vtkPointSmoothingFilter()
smooth1.SetInputConnection(textract.GetOutputPort())
smooth1.SetSmoothingModeToGeometric()
smooth1.SetNumberOfIterations(20)
smooth1.SetNumberOfSubIterations(10)
smooth1.SetPackingFactor(1.0)
smooth1.SetMaximumStepSize(0.01)
smooth1.SetNeighborhoodSize(24)
smooth1.EnableConstraintsOn()
smooth1.SetFixedAngle(45)
smooth1.SetBoundaryAngle(110)
smooth1.GenerateConstraintScalarsOn()
smooth1.Update()

glyph1 = vtkGlyph3D()
glyph1.SetInputConnection(smooth1.GetOutputPort())
glyph1.SetSourceConnection(sph.GetOutputPort())
glyph1.SetScaleModeToDataScalingOff()
glyph1.SetScaleFactor(0.025)

gMapper1 = vtkPolyDataMapper()
gMapper1.SetInputConnection(glyph1.GetOutputPort())
gMapper1.SetColorModeToMapScalars()
gMapper1.SetScalarModeToUsePointFieldData()
gMapper1.SetArrayAccessMode(1) #access by name
gMapper1.SetArrayName("Constraint Scalars")
gMapper1.SetScalarRange(0,2)

gActor1 = vtkActor()
gActor1.SetMapper(gMapper1)
gActor1.GetProperty().SetColor(1,1,1)
gActor1.GetProperty().SetOpacity(1)

# Now explicitly the Uniform behavior
smooth2 = vtkPointSmoothingFilter()
smooth2.SetInputConnection(textract.GetOutputPort())
smooth2.SetSmoothingModeToUniform()
smooth2.SetNumberOfIterations(40)
smooth2.SetNumberOfSubIterations(10)
smooth2.SetMaximumStepSize(0.001)
smooth2.SetNeighborhoodSize(24)
smooth2.SetPackingFactor(1.5)
smooth2.SetAttractionFactor(0.5)
smooth2.EnableConstraintsOn()
smooth2.SetFixedAngle(45)
smooth2.SetBoundaryAngle(105)
smooth2.GenerateConstraintScalarsOn()
smooth2.SetMotionConstraintToPlane()
smooth2.SetPlane(plane)
smooth2.Update()

glyph2 = vtkGlyph3D()
glyph2.SetInputConnection(smooth2.GetOutputPort())
glyph2.SetSourceConnection(sph.GetOutputPort())
glyph2.SetScaleModeToDataScalingOff()
glyph2.SetScaleFactor(0.025)

gMapper2 = vtkPolyDataMapper()
gMapper2.SetInputConnection(glyph2.GetOutputPort())
gMapper2.SetColorModeToMapScalars()
gMapper2.SetScalarModeToUsePointFieldData()
gMapper2.SetArrayAccessMode(1) #access by name
gMapper2.SetArrayName("Constraint Scalars")
gMapper2.SetScalarRange(0,2)

gActor2 = vtkActor()
gActor2.SetMapper(gMapper2)
gActor2.GetProperty().SetColor(1,1,1)
gActor2.GetProperty().SetOpacity(1)

# Now explicitly the Scalar behavior
smooth3 = vtkPointSmoothingFilter()
smooth3.SetInputConnection(textract.GetOutputPort())
smooth3.SetSmoothingModeToScalars()
smooth3.SetNumberOfIterations(100)
smooth3.SetNumberOfSubIterations(100)
smooth3.SetMaximumStepSize(0.0001)
smooth3.SetNeighborhoodSize(40)
smooth3.SetPackingFactor(1.)
smooth3.SetAttractionFactor(0.25)
smooth3.EnableConstraintsOn()
smooth3.SetFixedAngle(45)
smooth3.SetBoundaryAngle(100)
smooth3.GenerateConstraintScalarsOn()
smooth3.SetMotionConstraintToPlane()
smooth3.SetPlane(plane)
smooth3.Update()

glyph3 = vtkGlyph3D()
glyph3.SetInputConnection(smooth3.GetOutputPort())
glyph3.SetSourceConnection(sph.GetOutputPort())
glyph3.SetColorModeToColorByScalar()
glyph3.SetScaleFactor(1)

lut = vtkLookupTable()
lut.SetHueRange(0.6, 0)
lut.SetSaturationRange(1.0, 0)
lut.SetValueRange(0.5, 1.0)

gMapper3 = vtkPolyDataMapper()
gMapper3.SetInputConnection(glyph3.GetOutputPort())
gMapper3.SetScalarRange(smooth3.GetOutput().GetScalarRange())
gMapper3.SetLookupTable(lut)

gActor3 = vtkActor()
gActor3.SetMapper(gMapper3)
gActor3.GetProperty().SetColor(1,1,1)
gActor3.GetProperty().SetOpacity(1)

# Now explicitly the Tensor behavior
smooth4 = vtkPointSmoothingFilter()
smooth4.SetInputConnection(textract.GetOutputPort())
smooth4.SetSmoothingModeToTensors()
smooth4.SetNumberOfIterations(80)
smooth4.SetNumberOfSubIterations(20)
smooth4.SetMaximumStepSize(0.001)
smooth4.SetNeighborhoodSize(24)
smooth4.SetPackingFactor(1)
smooth4.SetAttractionFactor(0.25)
smooth4.EnableConstraintsOn()
smooth4.SetFixedAngle(45)
smooth4.SetBoundaryAngle(100)
smooth4.GenerateConstraintScalarsOn()
smooth4.SetMotionConstraintToPlane()
smooth4.SetPlane(plane)
smooth4.Update()

glyph4 = vtkTensorGlyph()
glyph4.SetInputConnection(smooth4.GetOutputPort())
glyph4.SetSourceConnection(sph.GetOutputPort())
glyph4.SetScaleFactor(0.1)

gMapper4 = vtkPolyDataMapper()
gMapper4.SetInputConnection(glyph4.GetOutputPort())
gMapper3.SetScalarRange(smooth4.GetOutput().GetScalarRange())

gActor4 = vtkActor()
gActor4.SetMapper(gMapper4)
gActor4.GetProperty().SetColor(1,1,1)
gActor4.GetProperty().SetOpacity(1)

# Now explicitly the Frame Field behavior
smooth5 = vtkPointSmoothingFilter()
smooth5.SetInputConnection(textract.GetOutputPort())
smooth5.SetSmoothingModeToTensors()
smooth5.SetNumberOfIterations(80)
smooth5.SetNumberOfSubIterations(10)
smooth5.SetMaximumStepSize(0.001)
smooth5.SetNeighborhoodSize(12)
smooth5.SetPackingFactor(1.5)
smooth5.SetAttractionFactor(0.25)
smooth5.EnableConstraintsOff()
smooth5.SetFixedAngle(45)
smooth5.SetBoundaryAngle(105)
smooth5.SetMotionConstraintToPlane()
smooth5.SetPlane(plane)
smooth5.Update()

glyph5 = vtkTensorGlyph()
glyph5.SetInputConnection(smooth5.GetOutputPort())
glyph5.SetSourceConnection(sph.GetOutputPort())
glyph5.SetScaleFactor(0.05)

gMapper5 = vtkPolyDataMapper()
gMapper5.SetInputConnection(glyph5.GetOutputPort())

gActor5 = vtkActor()
gActor5.SetMapper(gMapper5)
gActor5.GetProperty().SetColor(1,1,1)
gActor5.GetProperty().SetOpacity(1)

# A outline around the data helps for context
outline = vtkOutlineFilter()
outline.SetInputConnection(ptLoad.GetOutputPort())

outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(1,1,1)

# Create the RenderWindow, Renderer and both Actors
#
ren0 = vtkRenderer()
ren0.SetViewport(0, 0, 0.333, 0.5)
ren1 = vtkRenderer()
ren1.SetViewport(0.333, 0, 0.667, 0.5)
ren2 = vtkRenderer()
ren2.SetViewport(0.667, 0, 1, 0.5)
ren3 = vtkRenderer()
ren3.SetViewport(0, 0.5, 0.333, 1)
ren4 = vtkRenderer()
ren4.SetViewport(0.333, 0.5, 0.667, 1)
ren5 = vtkRenderer()
ren5.SetViewport(0.667, 0.5, 1, 1)

renWin = vtkRenderWindow()
renWin.AddRenderer(ren0)
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
renWin.AddRenderer(ren3)
renWin.AddRenderer(ren4)
renWin.AddRenderer(ren5)

iRen = vtkRenderWindowInteractor()
iRen.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
camera = vtkCamera()
camera.SetFocalPoint(0,0,0)
camera.SetPosition(normal)

ren0.AddActor(gActor0)
ren0.AddActor(outlineActor)
ren0.SetBackground(0,0,0)
ren0.SetActiveCamera(camera)
ren0.ResetCamera()

ren1.AddActor(gActor1)
ren1.AddActor(outlineActor)
ren1.SetBackground(0,0,0)
ren1.SetActiveCamera(camera)

ren2.AddActor(gActor2)
ren2.AddActor(outlineActor)
ren2.SetBackground(0,0,0)
ren2.SetActiveCamera(camera)

ren3.AddActor(gActor3)
ren3.AddActor(outlineActor)
ren3.SetBackground(0,0,0)
ren3.SetActiveCamera(camera)

ren4.AddActor(gActor4)
ren4.AddActor(outlineActor)
ren4.SetBackground(0,0,0)
ren4.SetActiveCamera(camera)

ren5.AddActor(gActor5)
ren5.AddActor(outlineActor)
ren5.SetBackground(0,0,0)
ren5.SetActiveCamera(camera)

renWin.SetSize(600, 400)

iRen.Initialize()
renWin.Render()

# Interact with the data
iRen.Start()
