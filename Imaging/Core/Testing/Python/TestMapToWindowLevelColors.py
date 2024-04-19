#!/usr/bin/env python
from vtkmodules.vtkImagingColor import vtkImageMapToWindowLevelColors
from vtkmodules.vtkImagingSources import vtkImageMandelbrotSource
from vtkmodules.vtkInteractionImage import vtkImageViewer
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

RANGE = 150
MAX_ITERATIONS_1 = RANGE
MAX_ITERATIONS_2 = RANGE
XRAD = 200
YRAD = 200

mandelbrot1 = vtkImageMandelbrotSource()
mandelbrot1.SetMaximumNumberOfIterations(150)
mandelbrot1.SetWholeExtent(0, XRAD - 1, 0, YRAD - 1, 0, 0)
mandelbrot1.SetSampleCX(1.3 / XRAD, 1.3 / XRAD, 1.3 / XRAD, 1.3 / XRAD)
mandelbrot1.SetOriginCX(-0.72, 0.22, 0.0, 0.0)
mandelbrot1.SetProjectionAxes(0, 1, 2)

mapToWL = vtkImageMapToWindowLevelColors()
mapToWL.SetInputConnection(mandelbrot1.GetOutputPort())
mapToWL.SetWindow(RANGE)
mapToWL.SetLevel(RANGE / 3.0)


# set the window/level to 255.0/127.5 to view full range
viewer = vtkImageViewer()
viewer.SetInputConnection(mapToWL.GetOutputPort())
viewer.SetColorWindow(255.0)
viewer.SetColorLevel(127.5)
viewer.Render()
