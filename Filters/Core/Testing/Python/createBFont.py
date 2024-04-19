#!/usr/bin/env python
from vtkmodules.vtkFiltersCore import vtkClipPolyData
from vtkmodules.vtkFiltersGeometry import vtkImageDataGeometryFilter
from vtkmodules.vtkIOImage import vtkPNMReader
from vtkmodules.vtkImagingGeneral import vtkImageGaussianSmooth
from vtkmodules.vtkRenderingCore import (
    vtkActor,
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

# Now create the RenderWindow, Renderer and Interactor
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
imageIn = vtkPNMReader()
imageIn.SetFileName(VTK_DATA_ROOT + "/Data/B.pgm")
gaussian = vtkImageGaussianSmooth()
gaussian.SetStandardDeviations(2,2)
gaussian.SetDimensionality(2)
gaussian.SetRadiusFactors(1,1)
gaussian.SetInputConnection(imageIn.GetOutputPort())
geometry = vtkImageDataGeometryFilter()
geometry.SetInputConnection(gaussian.GetOutputPort())
aClipper = vtkClipPolyData()
aClipper.SetInputConnection(geometry.GetOutputPort())
aClipper.SetValue(127.5)
aClipper.GenerateClipScalarsOff()
aClipper.InsideOutOn()
aClipper.GetOutput().GetPointData().CopyScalarsOff()
aClipper.Update()
mapper = vtkPolyDataMapper()
mapper.SetInputConnection(aClipper.GetOutputPort())
mapper.ScalarVisibilityOff()
letter = vtkActor()
letter.SetMapper(mapper)
ren1.AddActor(letter)
letter.GetProperty().SetDiffuseColor(0,0,0)
letter.GetProperty().SetRepresentationToWireframe()
ren1.SetBackground(1,1,1)
ren1.ResetCamera()
ren1.GetActiveCamera().Dolly(1.2)
ren1.ResetCameraClippingRange()
renWin.SetSize(320,320)
# render the image
#
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
