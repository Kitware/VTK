#!/usr/bin/env python

# This example demonstrates how to use boolean combinations of implicit
# functions to create a model of an ice cream cone.

import vtk
from vtk.util.colors import chocolate, mint

# Create implicit function primitives. These have been carefully
# placed to give the effect that we want. We are going to use various
# combinations of these functions to create the shape we want; for
# example, we use planes intersected with a cone (which is infinite in
# extent) to get a finite cone.
cone = vtk.vtkCone()
cone.SetAngle(20)
vertPlane = vtk.vtkPlane()
vertPlane.SetOrigin(.1, 0, 0)
vertPlane.SetNormal(-1, 0, 0)
basePlane = vtk.vtkPlane()
basePlane.SetOrigin(1.2, 0, 0)
basePlane.SetNormal(1, 0, 0)
iceCream = vtk.vtkSphere()
iceCream.SetCenter(1.333, 0, 0)
iceCream.SetRadius(0.5)
bite = vtk.vtkSphere()
bite.SetCenter(1.5, 0, 0.5)
bite.SetRadius(0.25)

# Combine primitives to build ice-cream cone. Clip the cone with planes.
theCone = vtk.vtkImplicitBoolean()
theCone.SetOperationTypeToIntersection()
theCone.AddFunction(cone)
theCone.AddFunction(vertPlane)
theCone.AddFunction(basePlane)

# Take a bite out of the ice cream.
theCream = vtk.vtkImplicitBoolean()
theCream.SetOperationTypeToDifference()
theCream.AddFunction(iceCream)
theCream.AddFunction(bite)

# The sample function generates a distance function from the implicit
# function (which in this case is the cone). This is then contoured to
# get a polygonal surface.
theConeSample = vtk.vtkSampleFunction()
theConeSample.SetImplicitFunction(theCone)
theConeSample.SetModelBounds(-1, 1.5, -1.25, 1.25, -1.25, 1.25)
theConeSample.SetSampleDimensions(60, 60, 60)
theConeSample.ComputeNormalsOff()
theConeSurface = vtk.vtkContourFilter()
theConeSurface.SetInput(theConeSample.GetOutput())
theConeSurface.SetValue(0, 0.0)
coneMapper = vtk.vtkPolyDataMapper()
coneMapper.SetInput(theConeSurface.GetOutput())
coneMapper.ScalarVisibilityOff()
coneActor = vtk.vtkActor()
coneActor.SetMapper(coneMapper)
coneActor.GetProperty().SetColor(chocolate)

# The same here for the ice cream.
theCreamSample = vtk.vtkSampleFunction()
theCreamSample.SetImplicitFunction(theCream)
theCreamSample.SetModelBounds(0, 2.5, -1.25, 1.25, -1.25, 1.25)
theCreamSample.SetSampleDimensions(60, 60, 60)
theCreamSample.ComputeNormalsOff()
theCreamSurface = vtk.vtkContourFilter()
theCreamSurface.SetInput(theCreamSample.GetOutput())
theCreamSurface.SetValue(0, 0.0)
creamMapper = vtk.vtkPolyDataMapper()
creamMapper.SetInput(theCreamSurface.GetOutput())
creamMapper.ScalarVisibilityOff()
creamActor = vtk.vtkActor()
creamActor.SetMapper(creamMapper)
creamActor.GetProperty().SetColor(mint)

# Create the usual rendering stuff
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
ren.AddActor(coneActor)
ren.AddActor(creamActor)
ren.SetBackground(1, 1, 1)
renWin.SetSize(250, 250)
ren.GetActiveCamera().Roll(90)
ren.GetActiveCamera().Dolly(1.5)
ren.ResetCameraClippingRange()

iren.Initialize()
renWin.Render()
iren.Start()
