#!/usr/bin/env python
from vtkmodules.vtkCommonExecutionModel import vtkImageToStructuredPoints
from vtkmodules.vtkFiltersCore import (
    vtkStripper,
    vtkThreshold,
)
from vtkmodules.vtkFiltersGeneral import (
    vtkLinkEdgels,
    vtkSubPixelPositionEdgels,
)
from vtkmodules.vtkFiltersGeometry import vtkGeometryFilter
from vtkmodules.vtkIOImage import vtkPNMReader
from vtkmodules.vtkImagingCore import (
    vtkImageCast,
    vtkImageConstantPad,
)
from vtkmodules.vtkImagingColor import vtkImageLuminance
from vtkmodules.vtkImagingGeneral import (
    vtkImageGaussianSmooth,
    vtkImageGradient,
)
from vtkmodules.vtkImagingMath import vtkImageMagnitude
from vtkmodules.vtkImagingMorphological import vtkImageNonMaximumSuppression
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

# Create the RenderWindow, Renderer and both Actors
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# load in the texture map
#
imageIn = vtkPNMReader()
imageIn.SetFileName(VTK_DATA_ROOT + "/Data/earth.ppm")
il = vtkImageLuminance()
il.SetInputConnection(imageIn.GetOutputPort())
ic = vtkImageCast()
ic.SetOutputScalarTypeToFloat()
ic.SetInputConnection(il.GetOutputPort())
# smooth the image
gs = vtkImageGaussianSmooth()
gs.SetInputConnection(ic.GetOutputPort())
gs.SetDimensionality(2)
gs.SetRadiusFactors(1,1,0)
# gradient the image
imgGradient = vtkImageGradient()
imgGradient.SetInputConnection(gs.GetOutputPort())
imgGradient.SetDimensionality(2)
imgMagnitude = vtkImageMagnitude()
imgMagnitude.SetInputConnection(imgGradient.GetOutputPort())
imgMagnitude.Update()
# non maximum suppression
nonMax = vtkImageNonMaximumSuppression()
nonMax.SetMagnitudeInputData(imgMagnitude.GetOutput())
nonMax.SetVectorInputData(imgGradient.GetOutput())
nonMax.SetDimensionality(2)
pad = vtkImageConstantPad()
pad.SetInputConnection(imgGradient.GetOutputPort())
pad.SetOutputNumberOfScalarComponents(3)
pad.SetConstant(0)
pad.Update()
i2sp1 = vtkImageToStructuredPoints()
i2sp1.SetInputConnection(nonMax.GetOutputPort())
i2sp1.SetVectorInputData(pad.GetOutput())
# link edgles
imgLink = vtkLinkEdgels()
imgLink.SetInputConnection(i2sp1.GetOutputPort())
imgLink.SetGradientThreshold(2)
# threshold links
thresholdEdgels = vtkThreshold()
thresholdEdgels.SetInputConnection(imgLink.GetOutputPort())
thresholdEdgels.SetThresholdFunction(vtkThreshold.THRESHOLD_UPPER)
thresholdEdgels.SetUpperThreshold(10.0)
thresholdEdgels.AllScalarsOff()
gf = vtkGeometryFilter()
gf.SetInputConnection(thresholdEdgels.GetOutputPort())
i2sp = vtkImageToStructuredPoints()
i2sp.SetInputConnection(imgMagnitude.GetOutputPort())
i2sp.SetVectorInputData(pad.GetOutput())
i2sp.Update()
# subpixel them
spe = vtkSubPixelPositionEdgels()
spe.SetInputConnection(gf.GetOutputPort())
spe.SetGradMapsData(i2sp.GetOutput())
strip = vtkStripper()
strip.SetInputConnection(spe.GetOutputPort())
dsm = vtkPolyDataMapper()
dsm.SetInputConnection(strip.GetOutputPort())
dsm.ScalarVisibilityOff()
planeActor = vtkActor()
planeActor.SetMapper(dsm)
planeActor.GetProperty().SetAmbient(1.0)
planeActor.GetProperty().SetDiffuse(0.0)
# Add the actors to the renderer, set the background and size
ren1.AddActor(planeActor)
renWin.SetSize(600,300)
# render the image
iren.Initialize()
renWin.Render()
ren1.GetActiveCamera().Zoom(2.8)
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
