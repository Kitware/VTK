#!/usr/bin/env python
# -*- coding: utf-8 -*-

from vtkmodules.vtkCommonCore import (
    vtkFloatArray,
    vtkMath,
    vtkPoints,
)
from vtkmodules.vtkCommonDataModel import (
    vtkCellArray,
    vtkPlane,
    vtkPolyData,
)
from vtkmodules.vtkFiltersCore import (
    vtkGlyph3D,
    vtkTensorGlyph,
)
from vtkmodules.vtkFiltersExtraction import vtkExtractTensorComponents
from vtkmodules.vtkFiltersModeling import vtkOutlineFilter
from vtkmodules.vtkFiltersPoints import vtkPointSmoothingFilter
from vtkmodules.vtkFiltersSources import vtkSphereSource
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
NPts = 1500
math = vtkMath()
math.RandomSeed(31415)

# Controls the plane normal and view plane normal
normal = [0,0,1]
center = [0,0,0]
plane = vtkPlane()
plane.SetOrigin(center)
plane.SetNormal(normal)

# Create a synthetic dataset
pts = vtkPoints()
pts.SetNumberOfPoints(NPts)
verts = vtkCellArray()
scalars = vtkFloatArray()
scalars.SetNumberOfTuples(NPts)
tensors = vtkFloatArray()
tensors.SetNumberOfComponents(6)
tensors.SetNumberOfTuples(NPts)

for i in range(0,NPts):
    pts.SetPoint(i,math.Random(-1,1),math.Random(-1,1),0.0)
    conn = [i]
    verts.InsertNextCell(1,conn)
    scalars.SetTuple1(i, math.Random(2,5))
    tensors.SetTuple6(i, math.Random(1,3),math.Random(-1,1),math.Random(-1,1),
                      math.Random(1,3),math.Random(-1,1),1)

pdata = vtkPolyData()
pdata.SetPoints(pts)
pdata.SetVerts(verts)
pdata.GetPointData().SetScalars(scalars)
pdata.GetPointData().SetTensors(tensors)

# Extract some tensor information
textract = vtkExtractTensorComponents()
textract.SetInputData(pdata)
textract.ExtractScalarsOn()
textract.ScalarIsDeterminant()
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
smooth0.SetInputData(pdata)
smooth0.SetNumberOfIterations(0) #sends input to output
smooth0.SetSmoothingModeToDefault()
smooth0.Update()

glyph0 = vtkGlyph3D()
glyph0.SetInputConnection(smooth0.GetOutputPort())
glyph0.SetSourceConnection(sph.GetOutputPort())
glyph0.SetScaleModeToDataScalingOff()
glyph0.SetScaleFactor(0.1)

gMapper0 = vtkPolyDataMapper()
gMapper0.SetInputConnection(glyph0.GetOutputPort())
gMapper0.ScalarVisibilityOff()

gActor0 = vtkActor()
gActor0.SetMapper(gMapper0)
gActor0.GetProperty().SetColor(1,1,1)
gActor0.GetProperty().SetOpacity(1)

# Now the geometric behavior
smooth1 = vtkPointSmoothingFilter()
smooth1.SetInputData(pdata)
smooth1.SetSmoothingModeToGeometric()
smooth1.SetNumberOfIterations(20)
smooth1.SetNumberOfSubIterations(100)
smooth1.SetMaximumStepSize(0.01)
smooth1.SetNeighborhoodSize(24)
smooth1.EnableConstraintsOff()
smooth1.SetFixedAngle(50)
smooth1.SetBoundaryAngle(100)
smooth1.GenerateConstraintScalarsOn()
smooth1.Update()

glyph1 = vtkGlyph3D()
glyph1.SetInputConnection(smooth1.GetOutputPort())
glyph1.SetSourceConnection(sph.GetOutputPort())
glyph1.SetScaleModeToDataScalingOff()
glyph1.SetScaleFactor(0.1)

gMapper1 = vtkPolyDataMapper()
gMapper1.SetInputConnection(glyph1.GetOutputPort())
gMapper1.SetColorModeToMapScalars()
gMapper1.SetScalarModeToUsePointFieldData()
gMapper1.SetArrayAccessMode(1) #by name
gMapper1.SetArrayName("Constraint Scalars")
gMapper1.SetScalarRange(0,2)

gActor1 = vtkActor()
gActor1.SetMapper(gMapper1)
gActor1.GetProperty().SetColor(1,1,1)
gActor1.GetProperty().SetOpacity(1)

# Now explicitly the Uniform behavior
smooth2 = vtkPointSmoothingFilter()
smooth2.SetInputData(pdata)
smooth2.SetSmoothingModeToUniform()
smooth2.SetNumberOfIterations(80)
smooth2.SetNumberOfSubIterations(10)
smooth2.EnableConstraintsOn()
smooth2.SetMaximumStepSize(0.01)
smooth2.SetNeighborhoodSize(20)
smooth2.GenerateConstraintScalarsOn()
smooth2.SetPackingFactor(1.5)
smooth2.Update()

glyph2 = vtkGlyph3D()
glyph2.SetInputConnection(smooth2.GetOutputPort())
glyph2.SetSourceConnection(sph.GetOutputPort())
glyph2.SetScaleModeToDataScalingOff()
glyph2.SetScaleFactor(0.1)
glyph2.Update()

gMapper2 = vtkPolyDataMapper()
gMapper2.SetInputConnection(glyph2.GetOutputPort())
gMapper2.SetScalarModeToUsePointFieldData()
gMapper2.SetArrayAccessMode(1) #by name
gMapper2.SetArrayName("Constraint Scalars")
gMapper2.SetScalarRange(0,2)

gActor2 = vtkActor()
gActor2.SetMapper(gMapper2)
gActor2.GetProperty().SetColor(1,1,1)
gActor2.GetProperty().SetOpacity(1)

# Now explicitly the Scalar behavior
smooth3 = vtkPointSmoothingFilter()
smooth3.SetInputData(pdata)
smooth3.SetSmoothingModeToScalars()
smooth3.SetNumberOfIterations(80)
smooth3.SetNumberOfSubIterations(20)
smooth3.SetMaximumStepSize(0.01)
smooth3.SetNeighborhoodSize(18)
smooth3.EnableConstraintsOff()
smooth3.SetFixedAngle(50)
smooth3.SetBoundaryAngle(100)
smooth3.GenerateConstraintScalarsOn()
smooth3.SetPackingFactor(1.5)
smooth3.Update()

glyph3 = vtkGlyph3D()
glyph3.SetInputConnection(smooth3.GetOutputPort())
glyph3.SetSourceConnection(sph.GetOutputPort())
glyph3.SetScaleFactor(0.01)

gMapper3 = vtkPolyDataMapper()
gMapper3.SetInputConnection(glyph3.GetOutputPort())
gMapper3.SetScalarRange(2,5)

gActor3 = vtkActor()
gActor3.SetMapper(gMapper3)
gActor3.GetProperty().SetColor(1,1,1)
gActor3.GetProperty().SetOpacity(1)

# Now explicitly the Tensor behavior
smooth4 = vtkPointSmoothingFilter()
smooth4.SetInputData(pdata)
smooth4.SetSmoothingModeToFrameField()
smooth4.SetNumberOfIterations(40)
smooth4.SetNumberOfSubIterations(10)
smooth4.SetMaximumStepSize(0.01)
smooth4.SetNeighborhoodSize(18)
smooth4.SetPackingFactor(0.75)
smooth4.SetMotionConstraintToPlane()
smooth4.SetPlane(plane)
smooth4.Update()

glyph4 = vtkTensorGlyph()
glyph4.SetInputConnection(smooth4.GetOutputPort())
glyph4.SetSourceConnection(sph.GetOutputPort())
glyph4.SetScaleFactor(0.025)

gMapper4 = vtkPolyDataMapper()
gMapper4.SetInputConnection(glyph4.GetOutputPort())
gMapper4.SetScalarRange(2,5)

gActor4 = vtkActor()
gActor4.SetMapper(gMapper4)
gActor4.GetProperty().SetColor(1,1,1)
gActor4.GetProperty().SetOpacity(1)

# Now explicitly the Frame Field behavior
smooth5 = vtkPointSmoothingFilter()
smooth5.SetInputData(pdata)
smooth5.SetSmoothingModeToFrameField()
smooth5.SetFrameFieldArray(tensors)
smooth5.SetNumberOfIterations(5000)
smooth5.SetNumberOfSubIterations(100)
smooth5.SetMaximumStepSize(0.00001)
smooth5.SetNeighborhoodSize(40)
smooth5.SetPackingFactor(0.25)
smooth5.SetAttractionFactor(0.25)
smooth5.SetMotionConstraintToPlane()
smooth5.SetPlane(plane)
smooth5.Update()

glyph5 = vtkTensorGlyph()
glyph5.SetInputConnection(smooth5.GetOutputPort())
glyph5.SetSourceConnection(sph.GetOutputPort())
glyph5.SetScaleFactor(0.025)

gMapper5 = vtkPolyDataMapper()
gMapper5.SetInputConnection(glyph5.GetOutputPort())
gMapper5.SetScalarRange(2,5)

gActor5 = vtkActor()
gActor5.SetMapper(gMapper5)
gActor5.GetProperty().SetColor(1,1,1)
gActor5.GetProperty().SetOpacity(1)

# A outline around the data helps for context
outline = vtkOutlineFilter()
outline.SetInputData(pdata)

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
