#!/usr/bin/env python

# This example demonstrates the use of the vtkTransformPolyDataFilter
# to reposition a 3D text string.

import vtk
from vtk.util.colors import *

# Define a Single Cube
Scalars = vtk.vtkFloatArray()
Scalars.InsertNextValue(1.0)
Scalars.InsertNextValue(0.0)
Scalars.InsertNextValue(0.0)
Scalars.InsertNextValue(1.0)
Scalars.InsertNextValue(0.0)
Scalars.InsertNextValue(0.0)
Scalars.InsertNextValue(0.0)
Scalars.InsertNextValue(0.0)

Points = vtk.vtkPoints()
Points.InsertNextPoint(0, 0, 0)
Points.InsertNextPoint(1, 0, 0)
Points.InsertNextPoint(1, 1, 0)
Points.InsertNextPoint(0, 1, 0)
Points.InsertNextPoint(0, 0, 1)
Points.InsertNextPoint(1, 0, 1)
Points.InsertNextPoint(1, 1, 1)
Points.InsertNextPoint(0, 1, 1)

Ids = vtk.vtkIdList()
Ids.InsertNextId(0)
Ids.InsertNextId(1)
Ids.InsertNextId(2)
Ids.InsertNextId(3)
Ids.InsertNextId(4)
Ids.InsertNextId(5)
Ids.InsertNextId(6)
Ids.InsertNextId(7)

Grid = vtk.vtkUnstructuredGrid()
Grid.Allocate(10, 10)
Grid.InsertNextCell(12, Ids)
Grid.SetPoints(Points)
Grid.GetPointData().SetScalars(Scalars)

# Find the triangles that lie along the 0.5 contour in this cube.
Marching = vtk.vtkContourFilter()
Marching.SetInput(Grid)
Marching.SetValue(0, 0.5)
Marching.Update()

# Extract the edges of the triangles just found.
triangleEdges = vtk.vtkExtractEdges()
triangleEdges.SetInput(Marching.GetOutput())
# Draw the edges as tubes instead of lines.  Also create the associated
# mapper and actor to display the tubes.
triangleEdgeTubes = vtk.vtkTubeFilter()
triangleEdgeTubes.SetInput(triangleEdges.GetOutput())
triangleEdgeTubes.SetRadius(.005)
triangleEdgeTubes.SetNumberOfSides(6)
triangleEdgeTubes.UseDefaultNormalOn()
triangleEdgeTubes.SetDefaultNormal(.577, .577, .577)
triangleEdgeMapper = vtk.vtkPolyDataMapper()
triangleEdgeMapper.SetInput(triangleEdgeTubes.GetOutput())
triangleEdgeMapper.ScalarVisibilityOff()
triangleEdgeActor = vtk.vtkActor()
triangleEdgeActor.SetMapper(triangleEdgeMapper)
triangleEdgeActor.GetProperty().SetDiffuseColor(lamp_black)
triangleEdgeActor.GetProperty().SetSpecular(.4)
triangleEdgeActor.GetProperty().SetSpecularPower(10)

# Shrink the triangles we found earlier.  Create the associated mapper
# and actor.  Set the opacity of the shrunken triangles.
aShrinker = vtk.vtkShrinkPolyData()
aShrinker.SetShrinkFactor(1)
aShrinker.SetInput(Marching.GetOutput())
aMapper = vtk.vtkPolyDataMapper()
aMapper.ScalarVisibilityOff()
aMapper.SetInput(aShrinker.GetOutput())
Triangles = vtk.vtkActor()
Triangles.SetMapper(aMapper)
Triangles.GetProperty().SetDiffuseColor(banana)
Triangles.GetProperty().SetOpacity(.6)

# Draw a cube the same size and at the same position as the one
# created previously.  Extract the edges because we only want to see
# the outline of the cube.  Pass the edges through a vtkTubeFilter so
# they are displayed as tubes rather than lines.
CubeModel = vtk.vtkCubeSource()
CubeModel.SetCenter(.5, .5, .5)
Edges = vtk.vtkExtractEdges()
Edges.SetInput(CubeModel.GetOutput())
Tubes = vtk.vtkTubeFilter()
Tubes.SetInput(Edges.GetOutput())
Tubes.SetRadius(.01)
Tubes.SetNumberOfSides(6)
Tubes.UseDefaultNormalOn()
Tubes.SetDefaultNormal(.577, .577, .577)
# Create the mapper and actor to display the cube edges.
TubeMapper = vtk.vtkPolyDataMapper()
TubeMapper.SetInput(Tubes.GetOutput())
CubeEdges = vtk.vtkActor()
CubeEdges.SetMapper(TubeMapper)
CubeEdges.GetProperty().SetDiffuseColor(khaki)
CubeEdges.GetProperty().SetSpecular(.4)
CubeEdges.GetProperty().SetSpecularPower(10)

# Create a sphere to use as a glyph source for vtkGlyph3D.
Sphere = vtk.vtkSphereSource()
Sphere.SetRadius(0.04)
Sphere.SetPhiResolution(20)
Sphere.SetThetaResolution(20)
# Remove the part of the cube with data values below 0.5.
ThresholdIn = vtk.vtkThresholdPoints()
ThresholdIn.SetInput(Grid)
ThresholdIn.ThresholdByUpper(.5)
# Display spheres at the vertices remaining in the cube data set after
# it was passed through vtkThresholdPoints.
Vertices = vtk.vtkGlyph3D()
Vertices.SetInput(ThresholdIn.GetOutput())
Vertices.SetSource(Sphere.GetOutput())
# Create a mapper and actor to display the glyphs.
SphereMapper = vtk.vtkPolyDataMapper()
SphereMapper.SetInput(Vertices.GetOutput())
SphereMapper.ScalarVisibilityOff()
CubeVertices = vtk.vtkActor()
CubeVertices.SetMapper(SphereMapper)
CubeVertices.GetProperty().SetDiffuseColor(tomato)
CubeVertices.GetProperty().SetDiffuseColor(tomato)

# Define the text for the label
caseLabel = vtk.vtkVectorText()
caseLabel.SetText("Case 1")

# Set up a transform to move the label to a new position.
aLabelTransform = vtk.vtkTransform()
aLabelTransform.Identity()
aLabelTransform.Translate(-0.2, 0, 1.25)
aLabelTransform.Scale(.05, .05, .05)

# Move the label to a new position.
labelTransform = vtk.vtkTransformPolyDataFilter()
labelTransform.SetTransform(aLabelTransform)
labelTransform.SetInput(caseLabel.GetOutput())
  
# Create a mapper and actor to display the text.
labelMapper = vtk.vtkPolyDataMapper()
labelMapper.SetInput(labelTransform.GetOutput())
 
labelActor = vtk.vtkActor()
labelActor.SetMapper(labelMapper)
 
# Define the base that the cube sits on.  Create its associated mapper
# and actor.  Set the position of the actor.
baseModel = vtk.vtkCubeSource()
baseModel.SetXLength(1.5)
baseModel.SetYLength(.01)
baseModel.SetZLength(1.5)
baseMapper = vtk.vtkPolyDataMapper()
baseMapper.SetInput(baseModel.GetOutput())
base = vtk.vtkActor()
base.SetMapper(baseMapper)
base.SetPosition(.5, -0.09, .5)

# Create the Renderer, RenderWindow, and RenderWindowInteractor
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
renWin.SetSize(640, 480)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer
ren.AddActor(triangleEdgeActor)
ren.AddActor(base)
ren.AddActor(labelActor)
ren.AddActor(CubeEdges)
ren.AddActor(CubeVertices)
ren.AddActor(Triangles)

# Set the background color.
ren.SetBackground(slate_grey)

# This sets up the right values for case12 of the marching cubes
# algorithm (routine translated from vtktesting/mccases.tcl).
def case12(scalars, caselabel, IN, OUT):
    scalars.InsertValue(0, OUT)
    scalars.InsertValue(1, IN)
    scalars.InsertValue(2, OUT)
    scalars.InsertValue(3, IN)
    scalars.InsertValue(4, IN)
    scalars.InsertValue(5, IN)
    scalars.InsertValue(6, OUT)
    scalars.InsertValue(7, OUT)
    if IN == 1:
        caselabel.SetText("Case 12 - 00111010")
    else:
        caselabel.SetText("Case 12 - 11000101")

# Set the scalar values for this case of marching cubes.
case12(Scalars, caseLabel, 0, 1)

# Force the grid to update.
Grid.Modified()

# Position the camera.
ren.GetActiveCamera().Dolly(1.2)
ren.GetActiveCamera().Azimuth(30)
ren.GetActiveCamera().Elevation(20)
ren.ResetCameraClippingRange()

iren.Initialize()
renWin.Render()
iren.Start()
