#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *
from libVTKImagingPython import *


# and some nice colors
from colors import *
# Now create the RenderWindow, Renderer and Interactor
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

image = vtkPNMReader()
image.SetFileName(VTK_DATA + "/B.pgm")

gaussian = vtkImageGaussianSmooth()
gaussian.SetStandardDeviations(2,2)
gaussian.SetDimensionality(2)
gaussian.SetRadiusFactors(1,1)
gaussian.SetInput(image.GetOutput())

toStructuredPoints = vtkImageToStructuredPoints()
toStructuredPoints.SetInput(gaussian.GetOutput())

geometry = vtkStructuredPointsGeometryFilter()
geometry.SetInput(toStructuredPoints.GetOutput())

aClipper = vtkClipPolyData()
aClipper.SetInput(geometry.GetOutput())
aClipper.SetValue(127.5)
aClipper.GenerateClipScalarsOff()
aClipper.InsideOutOn()
aClipper.GetOutput().GetPointData().CopyScalarsOff()
aClipper.Update()

mapper = vtkPolyDataMapper()
mapper.SetInput(aClipper.GetOutput())
mapper.ScalarVisibilityOff()

letter = vtkActor()
letter.SetMapper(mapper)

ren.AddActor(letter)
letter.GetProperty().SetDiffuseColor(0,0,0)
letter.GetProperty().SetRepresentationToWireframe()

ren.SetBackground(1,1,1)
ren.GetActiveCamera().Dolly(1.2)
ren.ResetCameraClippingRange()
renWin.SetSize(320,320)
iren.Initialize()

# render the image
#
renWin.Render()

iren.Start()
