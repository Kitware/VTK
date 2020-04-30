#!/usr/bin/env python
# -*- coding: utf-8 -*-

import vtk
import vtk.test.Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Simple test for debugging vtkPointSmoothingFilter

# Controls the plane normal and view plane normal
normal = [0,0,1]

# Create a synthetic dataset
pts = vtk.vtkPoints()
pts.SetNumberOfPoints(12)
pts.SetPoint(0, 0.25,0.0,0.0)
pts.SetPoint(1, 1.0,0.0,0.0)
pts.SetPoint(2, 0.707,0.707,0.0)
pts.SetPoint(3, 0.0,1.0,0.0)
pts.SetPoint(4, -0.707,0.707,0.0)
pts.SetPoint(5, -1.0,0.0,0.0)
pts.SetPoint(6, -0.707,-0.707,0.0)
pts.SetPoint(7, 0.0,-1.0,0.0)
pts.SetPoint(8, 0.707,-0.707,0.0)
pts.SetPoint(9, -0.25,0.0,0.0)
pts.SetPoint(10, 0.0,0.25,0.0)
pts.SetPoint(11, 0.0,-0.25,0.0)

verts = vtk.vtkCellArray()
conn = [0]
verts.InsertNextCell(1,conn)
conn = [1]
verts.InsertNextCell(1,conn)
conn = [2]
verts.InsertNextCell(1,conn)
conn = [3]
verts.InsertNextCell(1,conn)
conn = [4]
verts.InsertNextCell(1,conn)
conn = [5]
verts.InsertNextCell(1,conn)
conn = [6]
verts.InsertNextCell(1,conn)
conn = [7]
verts.InsertNextCell(1,conn)
conn = [8]
verts.InsertNextCell(1,conn)
conn = [9]
verts.InsertNextCell(1,conn)
conn = [10]
verts.InsertNextCell(1,conn)
conn = [11]
verts.InsertNextCell(1,conn)

# Now some scalars
scalars = vtk.vtkFloatArray()
scalars.SetNumberOfTuples(12)
scalars.SetTuple1(0, 2.0)
scalars.SetTuple1(1, 1.0)
scalars.SetTuple1(2, 1.0)
scalars.SetTuple1(3, 1.0)
scalars.SetTuple1(4, 1.0)
scalars.SetTuple1(5, 1.0)
scalars.SetTuple1(6, 1.0)
scalars.SetTuple1(7, 1.0)
scalars.SetTuple1(8, 1.0)
scalars.SetTuple1(9, 1.0)
scalars.SetTuple1(10, 1.0)
scalars.SetTuple1(11, 1.0)

# Now some tensors, with carefully selected rotations
matrixA = vtk.vtkActor()
matrixA.RotateZ(45)
matrixA.SetScale(4,1,1)
#print(matrixA.GetMatrix())

tensors = vtk.vtkFloatArray()
tensors.SetNumberOfComponents(9)
tensors.SetNumberOfTuples(12)
tensors.SetTuple9(0, 0.7,0.0,0.0, 0.0,0.1,0.0, 0.0,0.0,0.1)
tensors.SetTuple9(1, 0.1,0.0,0.0, 0.0,0.1,0.0, 0.0,0.0,0.1)
tensors.SetTuple9(2, 0.1,0.0,0.0, 0.0,0.1,0.0, 0.0,0.0,0.1)
tensors.SetTuple9(3, 0.667,0.0,0.0, 0.0,2,0.0, 0.0,0.0,0.667)
tensors.SetTuple9(4, 0.1,0.0,0.0, 0.0,0.1,0.0, 0.0,0.0,0.1)
tensors.SetTuple9(5, 0.2,0.0,0.0, 0.0,0.1,0.0, 0.0,0.0,0.1)
tensors.SetTuple9(6, 0.3,0.0,0.0, 0.0,0.1,0.0, 0.0,0.0,0.1)
tensors.SetTuple9(7, 0.4,0.0,0.0, 0.0,2.15,0.0, 0.0,0.0,0.1)
tensors.SetTuple9(8, 0.5,0.0,0.0, 0.0,0.1,0.0, 0.0,0.0,0.1)
tensors.SetTuple9(9, 2.47487,-2.47487,0.0, 1.41421,1.41421,0.0, 0.0,0.0,1.0)
#tensors.SetTuple9(10, 2.82843,2.82843,0.0, -0.707107,0.707107,0.0, 0.0,0.0,1.0)
tensors.SetTuple9(10, 3,0,0, 0,1,0, 0,0,1)
tensors.SetTuple9(11, 0.8,0.0,0.0, 0.0,0.1,0.0, 0.0,0.0,0.1)

pdata = vtk.vtkPolyData()
pdata.SetPoints(pts)
pdata.SetVerts(verts)
pdata.GetPointData().SetScalars(scalars)
pdata.GetPointData().SetTensors(tensors)

# Extract some tensor information
textract = vtk.vtkExtractTensorComponents()
textract.SetInputData(pdata)
textract.ExtractScalarsOn()
textract.ScalarIsDeterminant()
textract.PassTensorsToOutputOn()
textract.Update()

# Now smooth/pack the points in a variety of ways.
# We'll glyph with a transformed sphere
sph = vtk.vtkSphereSource()
sph.SetRadius(0.5)
sph.SetCenter(0.0, 0.0, 0.0)
sph.SetThetaResolution(16)
sph.SetPhiResolution(8)
sph.Update()

# First show the points unsmoothed
smooth0 = vtk.vtkPointSmoothingFilter()
smooth0.SetInputData(pdata)
smooth0.SetNumberOfIterations(0) #sends input to output
smooth0.SetSmoothingModeToDefault()
smooth0.Update()

glyph0 = vtk.vtkGlyph3D()
glyph0.SetInputConnection(smooth0.GetOutputPort())
glyph0.SetSourceConnection(sph.GetOutputPort())
glyph0.SetScaleModeToDataScalingOff()
glyph0.SetScaleFactor(0.1)

gMapper0 = vtk.vtkPolyDataMapper()
gMapper0.SetInputConnection(glyph0.GetOutputPort())
gMapper0.ScalarVisibilityOff()

gActor0 = vtk.vtkActor()
gActor0.SetMapper(gMapper0)
gActor0.GetProperty().SetColor(1,1,1)
gActor0.GetProperty().SetOpacity(1)

# Now the geometric behavior
smooth1 = vtk.vtkPointSmoothingFilter()
smooth1.SetInputData(pdata)
smooth1.SetSmoothingModeToGeometric()
smooth1.SetNumberOfIterations(20)
smooth1.SetNumberOfSubIterations(100)
smooth1.SetMaximumStepSize(0.01)
smooth1.SetNeighborhoodSize(24)
smooth1.EnableConstraintsOn()
smooth1.SetFixedAngle(50)
smooth1.SetBoundaryAngle(100)
smooth1.GenerateConstraintScalarsOn()
smooth1.Update()

glyph1 = vtk.vtkGlyph3D()
glyph1.SetInputConnection(smooth1.GetOutputPort())
glyph1.SetSourceConnection(sph.GetOutputPort())
glyph1.SetScaleModeToDataScalingOff()
glyph1.SetScaleFactor(0.1)

gMapper1 = vtk.vtkPolyDataMapper()
gMapper1.SetInputConnection(glyph1.GetOutputPort())
gMapper1.SetColorModeToMapScalars()
gMapper1.SetScalarModeToUsePointFieldData()
gMapper1.SetArrayAccessMode(1) #by name
gMapper1.SetArrayName("Constraint Scalars")
gMapper1.SetScalarRange(0,2)

gActor1 = vtk.vtkActor()
gActor1.SetMapper(gMapper1)
gActor1.GetProperty().SetColor(1,1,1)
gActor1.GetProperty().SetOpacity(1)

# Now explicitly the Uniform behavior
smooth2 = vtk.vtkPointSmoothingFilter()
smooth2.SetInputData(pdata)
smooth2.SetSmoothingModeToUniform()
smooth2.SetNumberOfIterations(20)
smooth2.SetNumberOfSubIterations(10)
smooth2.EnableConstraintsOn()
smooth2.SetMaximumStepSize(0.01)
smooth2.SetNeighborhoodSize(20)
smooth2.GenerateConstraintScalarsOn()
smooth2.SetPackingFactor(1.0)
smooth2.Update()

glyph2 = vtk.vtkGlyph3D()
glyph2.SetInputConnection(smooth2.GetOutputPort())
glyph2.SetSourceConnection(sph.GetOutputPort())
glyph2.SetScaleModeToDataScalingOff()
glyph2.SetScaleFactor(0.1)
glyph2.Update()

gMapper2 = vtk.vtkPolyDataMapper()
gMapper2.SetInputConnection(glyph2.GetOutputPort())
gMapper2.SetScalarModeToUsePointFieldData()
gMapper2.SetArrayAccessMode(1) #by name
gMapper2.SetArrayName("Constraint Scalars")
gMapper2.SetScalarRange(0,2)

gActor2 = vtk.vtkActor()
gActor2.SetMapper(gMapper2)
gActor2.GetProperty().SetColor(1,1,1)
gActor2.GetProperty().SetOpacity(1)

# Now explicitly the Scalar behavior
smooth3 = vtk.vtkPointSmoothingFilter()
smooth3.SetInputData(pdata)
smooth3.SetSmoothingModeToScalars()
smooth3.SetNumberOfIterations(30)
smooth3.SetNumberOfSubIterations(10)
smooth3.SetMaximumStepSize(0.01)
smooth3.SetNeighborhoodSize(18)
smooth1.EnableConstraintsOn()
smooth1.SetFixedAngle(50)
smooth1.SetBoundaryAngle(100)
smooth1.GenerateConstraintScalarsOn()
smooth3.Update()

glyph3 = vtk.vtkGlyph3D()
glyph3.SetInputConnection(smooth3.GetOutputPort())
glyph3.SetSourceConnection(sph.GetOutputPort())
#glyph3.SetScaleModeToDataScalingOff()
glyph3.SetScaleFactor(0.25)

gMapper3 = vtk.vtkPolyDataMapper()
gMapper3.SetInputConnection(glyph3.GetOutputPort())
gMapper3.SetScalarRange(0,2)

gActor3 = vtk.vtkActor()
gActor3.SetMapper(gMapper3)
gActor3.GetProperty().SetColor(1,1,1)
gActor3.GetProperty().SetOpacity(1)

# Now explicitly the Tensor behavior
smooth4 = vtk.vtkPointSmoothingFilter()
smooth4.SetInputData(pdata)
smooth4.SetSmoothingModeToTensors()
smooth4.SetNumberOfIterations(40)
smooth4.SetNumberOfSubIterations(10)
smooth4.SetMaximumStepSize(0.01)
smooth4.SetNeighborhoodSize(18)
smooth4.SetPackingFactor(1.5)
smooth4.Update()

glyph4 = vtk.vtkTensorGlyph()
glyph4.SetInputConnection(smooth4.GetOutputPort())
glyph4.SetSourceConnection(sph.GetOutputPort())
glyph4.SetScaleFactor(0.25)

gMapper4 = vtk.vtkPolyDataMapper()
gMapper4.SetInputConnection(glyph4.GetOutputPort())

gActor4 = vtk.vtkActor()
gActor4.SetMapper(gMapper4)
gActor4.GetProperty().SetColor(1,1,1)
gActor4.GetProperty().SetOpacity(1)

# Now explicitly the Frame Field behavior
smooth5 = vtk.vtkPointSmoothingFilter()
smooth5.SetInputData(pdata)
smooth5.SetSmoothingModeToFrameField()
smooth5.SetFrameFieldArray(tensors)
smooth5.SetMaximumStepSize(0.01)
smooth5.SetNumberOfIterations(1)
smooth5.SetNumberOfSubIterations(10)
smooth5.SetMaximumStepSize(0.01)
smooth5.SetNeighborhoodSize(18)
smooth4.SetPackingFactor(0.75)
smooth5.Update()

glyph5 = vtk.vtkTensorGlyph()
glyph5.SetInputConnection(smooth5.GetOutputPort())
glyph5.SetSourceConnection(sph.GetOutputPort())
glyph5.SetScaleFactor(0.25)

gMapper5 = vtk.vtkPolyDataMapper()
gMapper5.SetInputConnection(glyph5.GetOutputPort())

gActor5 = vtk.vtkActor()
gActor5.SetMapper(gMapper5)
gActor5.GetProperty().SetColor(1,1,1)
gActor5.GetProperty().SetOpacity(1)

# A outline around the data helps for context
outline = vtk.vtkOutlineFilter()
outline.SetInputData(pdata)

outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtk.vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(1,1,1)

# Create the RenderWindow, Renderer and both Actors
#
ren0 = vtk.vtkRenderer()
ren0.SetViewport(0, 0, 0.333, 0.5)
ren1 = vtk.vtkRenderer()
ren1.SetViewport(0.333, 0, 0.667, 0.5)
ren2 = vtk.vtkRenderer()
ren2.SetViewport(0.667, 0, 1, 0.5)
ren3 = vtk.vtkRenderer()
ren3.SetViewport(0, 0.5, 0.333, 1)
ren4 = vtk.vtkRenderer()
ren4.SetViewport(0.333, 0.5, 0.667, 1)
ren5 = vtk.vtkRenderer()
ren5.SetViewport(0.667, 0.5, 1, 1)

renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren0)
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
renWin.AddRenderer(ren3)
renWin.AddRenderer(ren4)
renWin.AddRenderer(ren5)

iRen = vtk.vtkRenderWindowInteractor()
iRen.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
camera = vtk.vtkCamera()
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
