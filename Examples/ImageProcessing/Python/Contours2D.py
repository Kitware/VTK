#!/usr/bin/env python

import vtk

# Quadric definition
quadric = vtk.vtkQuadric()
quadric.SetCoefficients(.5, 1, .2, 0, .1, 0, 0, .2, 0, 0)

sample = vtk.vtkSampleFunction()
sample.SetSampleDimensions(30, 30, 30)
sample.SetImplicitFunction(quadric)
sample.ComputeNormalsOff()
sample.Update()

extract = vtk.vtkExtractVOI()
extract.SetInput(sample.GetOutput())
extract.SetVOI(0, 29, 0, 29, 15, 15)
extract.SetSampleRate(1, 2, 3)

contours = vtk.vtkContourFilter()
contours.SetInput(extract.GetOutput())
contours.GenerateValues(13, 0.0, 1.2)

contMapper = vtk.vtkPolyDataMapper()
contMapper.SetInput(contours.GetOutput())
contMapper.SetScalarRange(0.0, 1.2)

contActor = vtk.vtkActor()
contActor.SetMapper(contMapper)

# Create outline
outline = vtk.vtkOutlineFilter()
outline.SetInput(sample.GetOutput())

outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInput(outline.GetOutput())

outlineActor = vtk.vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(0, 0, 0)

# create graphics objects
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren.SetBackground(1, 1, 1)
ren.AddActor(contActor)
ren.AddActor(outlineActor)

ren.GetActiveCamera().Zoom(1.5)

iren.Initialize()
renWin.Render()
iren.Start()
