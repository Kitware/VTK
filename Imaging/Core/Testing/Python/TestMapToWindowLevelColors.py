#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

RANGE = 150
MAX_ITERATIONS_1 = RANGE
MAX_ITERATIONS_2 = RANGE
XRAD = 200
YRAD = 200

mandelbrot1 = vtk.vtkImageMandelbrotSource()
mandelbrot1.SetMaximumNumberOfIterations(150)
mandelbrot1.SetWholeExtent(0, XRAD - 1, 0, YRAD - 1, 0, 0)
mandelbrot1.SetSampleCX(1.3 / XRAD, 1.3 / XRAD, 1.3 / XRAD, 1.3 / XRAD)
mandelbrot1.SetOriginCX(-0.72, 0.22, 0.0, 0.0)
mandelbrot1.SetProjectionAxes(0, 1, 2)

mapToWL = vtk.vtkImageMapToWindowLevelColors()
mapToWL.SetInputConnection(mandelbrot1.GetOutputPort())
mapToWL.SetWindow(RANGE)
mapToWL.SetLevel(RANGE / 3.0)


# set the window/level to 255.0/127.5 to view full range
viewer = vtk.vtkImageViewer()
viewer.SetInputConnection(mapToWL.GetOutputPort())
viewer.SetColorWindow(255.0)
viewer.SetColorLevel(127.5)
viewer.Render()
