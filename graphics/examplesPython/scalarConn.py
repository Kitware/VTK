#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

# Demonstrate use of scalar connectivity


# Quadric definition
quadric = vtkQuadric()
quadric.SetCoefficients(.5,1,.2,0,.1,0,0,.2,0,0)

sample = vtkSampleFunction()
sample.SetSampleDimensions(30,30,30)
sample.SetImplicitFunction(quadric)
sample.Update()
#sample.Print()
sample.ComputeNormalsOff()

# Extract cells that contains isosurface of interest
conn = vtkConnectivityFilter()
conn.SetInput(sample.GetOutput())
conn.ScalarConnectivityOn()
conn.SetScalarRange(0.6,0.6)
conn.SetExtractionModeToCellSeededRegions()
conn.AddSeed(105)

# Create a surface 
contours = vtkContourFilter()
contours.SetInput(conn.GetOutput())
#  contours SetInput [sample GetOutput]
contours.GenerateValues(5,0.0,1.2)

contMapper = vtkDataSetMapper()
#  contMapper SetInput [contours GetOutput]
contMapper.SetInput(conn.GetOutput())
contMapper.SetScalarRange(0.0,1.2)

contActor = vtkActor()
contActor.SetMapper(contMapper)

# Create outline
outline = vtkOutlineFilter()
outline.SetInput(sample.GetOutput())

outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInput(outline.GetOutput())

outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(0,0,0)

# Graphics
# create a window to render into
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
renWin.SetSize(400,400)

# create a renderer

# interactiver renderer catches mouse events (optional)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren.SetBackground(1,1,1)
ren.AddActor(contActor)
ren.AddActor(outlineActor)
ren.GetActiveCamera().Zoom(1.4)
iren.Initialize()

renWin.SetFileName("valid/scalarConn.ppm")
iren.Start()
