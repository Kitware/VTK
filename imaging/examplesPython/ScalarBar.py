#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


import signal
from vtkpython import *

# This example demonstrates how to use cell data as well as the programmable attribute 
# filter. Example randomly colors cells with scalar values.

# create pipeline
#
# create sphere to color
sphere = vtkSphereSource()
sphere.SetThetaResolution(20)
sphere.SetPhiResolution(40)

# Compute random scalars (colors) for each cell
randomColors = vtkProgrammableAttributeDataFilter()
randomColors.SetInput(sphere.GetOutput())

def colorCells():
   randomColorGenerator = vtkMath()
   input = randomColors.GetInput()
   output = randomColors.GetOutput()
   numCells = input.GetNumberOfCells()
   colors = vtkScalars()
   colors.SetNumberOfScalars(numCells)

   for i in range(0,numCells):
      colors.SetScalar(i,randomColorGenerator.Random(0,1))
     
   output.GetCellData().CopyScalarsOff()
   output.GetCellData().PassData(input.GetCellData())
   output.GetCellData().SetScalars(colors)

   colors.Delete() #reference.counting(-,it's.ok)
   randomColorGenerator.Delete()
 
randomColors.SetExecuteMethod(colorCells)

# mapper and actor
mapper = vtkPolyDataMapper()
mapper.SetInput(randomColors.GetPolyDataOutput())
mapper.SetScalarRange(randomColors.GetPolyDataOutput().GetScalarRange())
sphereActor = vtkActor()
sphereActor.SetMapper(mapper)

# Create a scalar bar
scalarBar = vtkScalarBarActor()
scalarBar.SetLookupTable(mapper.GetLookupTable())
scalarBar.SetTitle("Temperature")
scalarBar.GetPositionCoordinate().SetCoordinateSystemToNormalizedViewport()
scalarBar.GetPositionCoordinate().SetValue(0.1,0.01)
scalarBar.SetOrientationToHorizontal()
scalarBar.SetWidth(0.8)
scalarBar.SetHeight(0.17)

# Create graphics stuff
# Create the RenderWindow, Renderer and both Actors
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren.AddActor(sphereActor)
ren.AddActor2D(scalarBar)
renWin.SetSize(500,500)

# render the image
#
ren.GetActiveCamera().Zoom(1.5)
renWin.Render()
scalarBar.SetNumberOfLabels(8)
renWin.Render()

#renWin.SetFileName("ScalarBar.tcl.ppm")
#renWin.SaveImageAsPPM()

signal.pause()
