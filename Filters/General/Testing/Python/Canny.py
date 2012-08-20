#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create the RenderWindow, Renderer and both Actors
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# load in the texture map
#
imageIn = vtk.vtkPNMReader()
imageIn.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/earth.ppm")
il = vtk.vtkImageLuminance()
il.SetInputConnection(imageIn.GetOutputPort())
ic = vtk.vtkImageCast()
ic.SetOutputScalarTypeToFloat()
ic.SetInputConnection(il.GetOutputPort())
# smooth the image
gs = vtk.vtkImageGaussianSmooth()
gs.SetInputConnection(ic.GetOutputPort())
gs.SetDimensionality(2)
gs.SetRadiusFactors(1,1,0)
# gradient the image
imgGradient = vtk.vtkImageGradient()
imgGradient.SetInputConnection(gs.GetOutputPort())
imgGradient.SetDimensionality(2)
imgMagnitude = vtk.vtkImageMagnitude()
imgMagnitude.SetInputConnection(imgGradient.GetOutputPort())
imgMagnitude.Update()
# non maximum suppression
nonMax = vtk.vtkImageNonMaximumSuppression()
nonMax.SetMagnitudeInputData(imgMagnitude.GetOutput())
nonMax.SetVectorInputData(imgGradient.GetOutput())
nonMax.SetDimensionality(2)
pad = vtk.vtkImageConstantPad()
pad.SetInputConnection(imgGradient.GetOutputPort())
pad.SetOutputNumberOfScalarComponents(3)
pad.SetConstant(0)
pad.Update()
i2sp1 = vtk.vtkImageToStructuredPoints()
i2sp1.SetInputConnection(nonMax.GetOutputPort())
i2sp1.SetVectorInputData(pad.GetOutput())
# link edgles
imgLink = vtk.vtkLinkEdgels()
imgLink.SetInputConnection(i2sp1.GetOutputPort())
imgLink.SetGradientThreshold(2)
# threshold links
thresholdEdgels = vtk.vtkThreshold()
thresholdEdgels.SetInputConnection(imgLink.GetOutputPort())
thresholdEdgels.ThresholdByUpper(10)
thresholdEdgels.AllScalarsOff()
gf = vtk.vtkGeometryFilter()
gf.SetInputConnection(thresholdEdgels.GetOutputPort())
i2sp = vtk.vtkImageToStructuredPoints()
i2sp.SetInputConnection(imgMagnitude.GetOutputPort())
i2sp.SetVectorInputData(pad.GetOutput())
i2sp.Update()
# subpixel them
spe = vtk.vtkSubPixelPositionEdgels()
spe.SetInputConnection(gf.GetOutputPort())
spe.SetGradMapsData(i2sp.GetOutput())
strip = vtk.vtkStripper()
strip.SetInputConnection(spe.GetOutputPort())
dsm = vtk.vtkPolyDataMapper()
dsm.SetInputConnection(strip.GetOutputPort())
dsm.ScalarVisibilityOff()
planeActor = vtk.vtkActor()
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
