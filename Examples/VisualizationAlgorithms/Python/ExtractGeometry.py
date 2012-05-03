#!/usr/bin/env python

# This example shows how to extract a piece of a dataset using an
# implicit function. In this case the implicit function is formed by
# the boolean combination of two ellipsoids.

import vtk

# Here we create two ellipsoidal implicit functions and boolean them
# together tto form a "cross" shaped implicit function.
quadric = vtk.vtkQuadric()
quadric.SetCoefficients(.5, 1, .2, 0, .1, 0, 0, .2, 0, 0)
sample = vtk.vtkSampleFunction()
sample.SetSampleDimensions(50, 50, 50)
sample.SetImplicitFunction(quadric)
sample.ComputeNormalsOff()
trans = vtk.vtkTransform()
trans.Scale(1, .5, .333)
sphere = vtk.vtkSphere()
sphere.SetRadius(0.25)
sphere.SetTransform(trans)
trans2 = vtk.vtkTransform()
trans2.Scale(.25, .5, 1.0)
sphere2 = vtk.vtkSphere()
sphere2.SetRadius(0.25)
sphere2.SetTransform(trans2)
union = vtk.vtkImplicitBoolean()
union.AddFunction(sphere)
union.AddFunction(sphere2)
union.SetOperationType(0) #union

# Here is where it gets interesting. The implicit function is used to
# extract those cells completely inside the function. They are then
# shrunk to help show what was extracted.
extract = vtk.vtkExtractGeometry()
extract.SetInputConnection(sample.GetOutputPort())
extract.SetImplicitFunction(union)
shrink = vtk.vtkShrinkFilter()
shrink.SetInputConnection(extract.GetOutputPort())
shrink.SetShrinkFactor(0.5)
dataMapper = vtk.vtkDataSetMapper()
dataMapper.SetInputConnection(shrink.GetOutputPort())
dataActor = vtk.vtkActor()
dataActor.SetMapper(dataMapper)

# The outline gives context to the original data.
outline = vtk.vtkOutlineFilter()
outline.SetInputConnection(sample.GetOutputPort())
outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = vtk.vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineProp = outlineActor.GetProperty()
outlineProp.SetColor(0, 0, 0)

# The usual rendering stuff is created.
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
ren.AddActor(outlineActor)
ren.AddActor(dataActor)
ren.SetBackground(1, 1, 1)
renWin.SetSize(500, 500)
ren.ResetCamera()
ren.GetActiveCamera().Zoom(1.5)

iren.Initialize()
renWin.Render()
iren.Start()
