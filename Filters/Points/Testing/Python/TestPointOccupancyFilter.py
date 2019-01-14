#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# The resolution of the masking volume
res = 15

# Parameters for debugging
math = vtk.vtkMath()
math.RandomSeed(31415)

# create pipeline
#
# A set of points at the extreme positions
pts = vtk.vtkPoints()
pts.SetNumberOfPoints(9)
pts.SetPoint(0, 0,0,0)
pts.SetPoint(1, -1,-1,-1)
pts.SetPoint(2,  1,-1,-1)
pts.SetPoint(3, -1, 1,-1)
pts.SetPoint(4,  1, 1,-1)
pts.SetPoint(5, -1,-1, 1)
pts.SetPoint(6,  1,-1, 1)
pts.SetPoint(7, -1, 1, 1)
pts.SetPoint(8,  1, 1, 1)

ptsPD = vtk.vtkPolyData()
ptsPD.SetPoints(pts);

# Generate occupancy mask
occ = vtk.vtkPointOccupancyFilter()
occ.SetInputData(ptsPD)
occ.SetSampleDimensions(res,res+2,res+4)
occ.SetOccupiedValue(255)

# Display the result: outline plus surface
outline = vtk.vtkOutlineFilter()
outline.SetInputConnection(occ.GetOutputPort())

outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtk.vtkActor()
outlineActor.SetMapper(outlineMapper);

surface = vtk.vtkThreshold()
surface.SetInputConnection(occ.GetOutputPort())
surface.ThresholdByUpper(1)
surface.AllScalarsOff()

surfaceMapper = vtk.vtkDataSetMapper()
surfaceMapper.SetInputConnection(surface.GetOutputPort())
surfaceMapper.ScalarVisibilityOff()

surfaceActor = vtk.vtkActor()
surfaceActor.SetMapper(surfaceMapper);

points = vtk.vtkMaskPointsFilter()
points.SetInputData(ptsPD)
points.SetMaskConnection(occ.GetOutputPort())
points.GenerateVerticesOn()

pointsMapper = vtk.vtkPolyDataMapper()
pointsMapper.SetInputConnection(points.GetOutputPort())

pointsActor = vtk.vtkActor()
pointsActor.SetMapper(pointsMapper);
pointsActor.GetProperty().SetPointSize(3)
pointsActor.GetProperty().SetColor(0,1,0)

# Create the RenderWindow, Renderer and both Actors
#
ren0 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren0)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
#ren0.AddActor(outlineActor)
ren0.AddActor(surfaceActor)
#ren0.AddActor(pointsActor)
ren0.SetBackground(0,0,0)

renWin.SetSize(300,300)

cam = ren0.GetActiveCamera()
cam.SetFocalPoint(0,0,0)
cam.SetPosition(1,1,1)
ren0.ResetCamera()

iren.Initialize()

# render the image
#
renWin.Render()

iren.Start()
