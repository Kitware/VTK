#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Parameters for debugging
NPts = 100000

# create pipeline
#
points = vtk.vtkPointSource()
points.SetNumberOfPoints(NPts)
points.SetRadius(5)

# create some scalars based on implicit function
# Create a cylinder
cyl = vtk.vtkCylinder()
cyl.SetCenter(0,0,0)
cyl.SetRadius(0.1)

# Generate scalars and vector
sample = vtk.vtkSampleImplicitFunctionFilter()
sample.SetInputConnection(points.GetOutputPort())
sample.SetImplicitFunction(cyl)
sample.Update()
print(sample.GetOutput().GetScalarRange())

# Draw the points
sampleMapper = vtk.vtkPointGaussianMapper()
sampleMapper.SetInputConnection(sample.GetOutputPort(0))
sampleMapper.EmissiveOff()
sampleMapper.SetScaleFactor(0.0)
sampleMapper.SetScalarRange(0,20)

sampleActor = vtk.vtkActor()
sampleActor.SetMapper(sampleMapper)

# Create an outline
outline = vtk.vtkOutlineFilter()
outline.SetInputConnection(sample.GetOutputPort())

outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtk.vtkActor()
outlineActor.SetMapper(outlineMapper)

# Create the RenderWindow, Renderer and both Actors
#
ren0 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren0)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren0.AddActor(sampleActor)
ren0.AddActor(outlineActor)
ren0.SetBackground(0.1, 0.2, 0.4)

renWin.SetSize(250,250)

cam = ren0.GetActiveCamera()
cam.SetFocalPoint(0,0,0)
cam.SetPosition(1,1,1)
ren0.ResetCamera()

iren.Initialize()

# render the image
#
renWin.Render()

#iren.Start()
