#!/usr/bin/env python

# This example demonstrates the use of vtkCardinalSpline.
# It creates random points and connects them with a spline

import vtk
from vtk.util.colors import tomato, banana

# This will be used later to get random numbers.
math = vtk.vtkMath()

# Total number of points.
numberOfInputPoints = 10

# One spline for each direction.
aSplineX = vtk.vtkCardinalSpline()
aSplineY = vtk.vtkCardinalSpline()
aSplineZ = vtk.vtkCardinalSpline()

# Generate random (pivot) points and add the corresponding
# coordinates to the splines.
# aSplineX will interpolate the x values of the points
# aSplineY will interpolate the y values of the points
# aSplineZ will interpolate the z values of the points
inputPoints = vtk.vtkPoints()
for i in range(0, numberOfInputPoints):
    x = math.Random(0, 1)
    y = math.Random(0, 1)
    z = math.Random(0, 1)
    aSplineX.AddPoint(i, x)
    aSplineY.AddPoint(i, y)
    aSplineZ.AddPoint(i, z)
    inputPoints.InsertPoint(i, x, y, z)
 

# The following section will create glyphs for the pivot points
# in order to make the effect of the spline more clear.

# Create a polydata to be glyphed.
inputData = vtk.vtkPolyData()
inputData.SetPoints(inputPoints)

# Use sphere as glyph source.
balls = vtk.vtkSphereSource()
balls.SetRadius(.01)
balls.SetPhiResolution(10)
balls.SetThetaResolution(10)

glyphPoints = vtk.vtkGlyph3D()
glyphPoints.SetInput(inputData)
glyphPoints.SetSource(balls.GetOutput())

glyphMapper = vtk.vtkPolyDataMapper()
glyphMapper.SetInput(glyphPoints.GetOutput())

glyph = vtk.vtkActor()
glyph.SetMapper(glyphMapper)
glyph.GetProperty().SetDiffuseColor(tomato)
glyph.GetProperty().SetSpecular(.3)
glyph.GetProperty().SetSpecularPower(30)

# Generate the polyline for the spline.
points = vtk.vtkPoints()
profileData = vtk.vtkPolyData()

# Number of points on the spline
numberOfOutputPoints = 400

# Interpolate x, y and z by using the three spline filters and
# create new points
for i in range(0, numberOfOutputPoints):
    t = (numberOfInputPoints-1.0)/(numberOfOutputPoints-1.0)*i
    points.InsertPoint(i, aSplineX.Evaluate(t), aSplineY.Evaluate(t),
                       aSplineZ.Evaluate(t))
 

# Create the polyline.
lines = vtk.vtkCellArray()
lines.InsertNextCell(numberOfOutputPoints)
for i in range(0, numberOfOutputPoints):
    lines.InsertCellPoint(i)
 
profileData.SetPoints(points)
profileData.SetLines(lines)

# Add thickness to the resulting line.
profileTubes = vtk.vtkTubeFilter()
profileTubes.SetNumberOfSides(8)
profileTubes.SetInput(profileData)
profileTubes.SetRadius(.005)

profileMapper = vtk.vtkPolyDataMapper()
profileMapper.SetInput(profileTubes.GetOutput())

profile = vtk.vtkActor()
profile.SetMapper(profileMapper)
profile.GetProperty().SetDiffuseColor(banana)
profile.GetProperty().SetSpecular(.3)
profile.GetProperty().SetSpecularPower(30)

# Now create the RenderWindow, Renderer and Interactor
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors
ren.AddActor(glyph)
ren.AddActor(profile)

renWin.SetSize(500, 500)

iren.Initialize()
renWin.Render()
iren.Start()
