#!/usr/bin/env python

# This example demonstrates how to use a programmable filter and how
# to use the special vtkDataSetToDataSet::GetOutput() methods

import vtk
from math import *

# We create a 100 by 100 point plane to sample
plane = vtk.vtkPlaneSource()
plane.SetXResolution(100)
plane.SetYResolution(100)

# We transform the plane by a factor of 10 on X and Y
transform = vtk.vtkTransform()
transform.Scale(10, 10, 1)
transF = vtk.vtkTransformPolyDataFilter()
transF.SetInput(plane.GetOutput())
transF.SetTransform(transform)

# Compute Bessel function and derivatives. We'll use a programmable filter
# for this. Note the unusual GetInput() & GetOutput() methods.
besselF = vtk.vtkProgrammableFilter()
besselF.SetInput(transF.GetOutput())

# The SetExecuteMethod takes a Python function as an argument
# In here is where all the processing is done.
def bessel():
    input = besselF.GetPolyDataInput()
    numPts = input.GetNumberOfPoints()
    newPts = vtk.vtkPoints()
    derivs = vtk.vtkFloatArray()

    for i in range(0, numPts):
        x = input.GetPoint(i)
        x0, x1 = x[:2]

        r = sqrt(x0*x0+x1*x1)
        x2 = exp(-r)*cos(10.0*r)
        deriv = -exp(-r)*(cos(10.0*r)+10.0*sin(10.0*r))

        newPts.InsertPoint(i, x0, x1, x2)
        derivs.InsertValue(i, deriv) 

    besselF.GetPolyDataOutput().CopyStructure(input)
    besselF.GetPolyDataOutput().SetPoints(newPts)
    besselF.GetPolyDataOutput().GetPointData().SetScalars(derivs)

besselF.SetExecuteMethod(bessel) 

# We warp the plane based on the scalar values calculated above
warp = vtk.vtkWarpScalar()
warp.SetInput(besselF.GetPolyDataOutput())
warp.XYPlaneOn()
warp.SetScaleFactor(0.5)


# We create a mapper and actor as usual. In the case we adjust the
# scalar range of the mapper to match that of the computed scalars
mapper = vtk.vtkPolyDataMapper()
mapper.SetInput(warp.GetPolyDataOutput())
mapper.SetScalarRange(besselF.GetPolyDataOutput().GetScalarRange())
carpet = vtk.vtkActor()
carpet.SetMapper(mapper)

# Create the RenderWindow, Renderer
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren.AddActor(carpet)
renWin.SetSize(500, 500)

ren.GetActiveCamera().Zoom(1.5)

iren.Initialize()
renWin.Render()
iren.Start()
