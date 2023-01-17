#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import vtkCylinder
from vtkmodules.vtkFiltersGeneral import vtkSampleImplicitFunctionFilter
from vtkmodules.vtkFiltersModeling import vtkOutlineFilter
from vtkmodules.vtkFiltersSources import vtkPointSource
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPointGaussianMapper,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Parameters for debugging
NPts = 100000

# create pipeline
#
points = vtkPointSource()
points.SetNumberOfPoints(NPts)
points.SetRadius(5)

# create some scalars based on implicit function
# Create a cylinder
cyl = vtkCylinder()
cyl.SetCenter(0,0,0)
cyl.SetRadius(0.1)

# Generate scalars and vector
sample = vtkSampleImplicitFunctionFilter()
sample.SetInputConnection(points.GetOutputPort())
sample.SetImplicitFunction(cyl)
sample.Update()
print(sample.GetOutput().GetScalarRange())

# Draw the points
sampleMapper = vtkPointGaussianMapper()
sampleMapper.SetInputConnection(sample.GetOutputPort(0))
sampleMapper.EmissiveOff()
sampleMapper.SetScaleFactor(0.0)
sampleMapper.SetScalarRange(0,20)

sampleActor = vtkActor()
sampleActor.SetMapper(sampleMapper)

# Create an outline
outline = vtkOutlineFilter()
outline.SetInputConnection(sample.GetOutputPort())

outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)

# Create the RenderWindow, Renderer and both Actors
#
ren0 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren0)
iren = vtkRenderWindowInteractor()
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
