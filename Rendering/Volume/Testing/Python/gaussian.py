#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)

renWin.SetSize(300, 300)

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

camera = vtk.vtkCamera()
camera.ParallelProjectionOn()
camera.SetViewUp(0, 1, 0)
camera.SetFocalPoint(12, 10.5, 15)
camera.SetPosition(-70, 15, 34)
camera.ComputeViewPlaneNormal()
ren1.SetActiveCamera(camera)
# Create the reader for the data
# vtkStructuredPointsReader reader
reader = vtk.vtkGaussianCubeReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/m4_TotalDensity.cube")
reader.SetHBScale(1.1)
reader.SetBScale(10)
reader.Update()

range = reader.GetGridOutput().GetPointData().GetScalars().GetRange()

min = range[0]
max = range[1]

readerSS = vtk.vtkImageShiftScale()
readerSS.SetInputData(reader.GetGridOutput())
readerSS.SetShift(min * -1)
readerSS.SetScale(255 / (max - min))
readerSS.SetOutputScalarTypeToUnsignedChar()

bounds = vtk.vtkOutlineFilter()
bounds.SetInputData(reader.GetGridOutput())

boundsMapper = vtk.vtkPolyDataMapper()
boundsMapper.SetInputConnection(bounds.GetOutputPort())

boundsActor = vtk.vtkActor()
boundsActor.SetMapper(boundsMapper)
boundsActor.GetProperty().SetColor(0, 0, 0)

contour = vtk.vtkContourFilter()
contour.SetInputData(reader.GetGridOutput())
contour.GenerateValues(5, 0, .05)

contourMapper = vtk.vtkPolyDataMapper()
contourMapper.SetInputConnection(contour.GetOutputPort())
contourMapper.SetScalarRange(0, .1)
contourMapper.GetLookupTable().SetHueRange(0.32, 0)

contourActor = vtk.vtkActor()
contourActor.SetMapper(contourMapper)
contourActor.GetProperty().SetOpacity(.5)

# Create transfer mapping scalar value to opacity
opacityTransferFunction = vtk.vtkPiecewiseFunction()
opacityTransferFunction.AddPoint(0, 0.01)
opacityTransferFunction.AddPoint(255, 0.35)
opacityTransferFunction.ClampingOn()

# Create transfer mapping scalar value to color
colorTransferFunction = vtk.vtkColorTransferFunction()
colorTransferFunction.AddHSVPoint(0.0, 0.66, 1.0, 1.0)
colorTransferFunction.AddHSVPoint(50.0, 0.33, 1.0, 1.0)
colorTransferFunction.AddHSVPoint(100.0, 0.00, 1.0, 1.0)

# The property describes how the data will look
volumeProperty = vtk.vtkVolumeProperty()
volumeProperty.SetColor(colorTransferFunction)
volumeProperty.SetScalarOpacity(opacityTransferFunction)
volumeProperty.SetInterpolationTypeToLinear()

# The mapper knows how to render the data
volumeMapper = vtk.vtkFixedPointVolumeRayCastMapper()
volumeMapper.SetInputConnection(readerSS.GetOutputPort())

# The volume holds the mapper and the property and
# can be used to position/orient the volume
volume = vtk.vtkVolume()
volume.SetMapper(volumeMapper)
volume.SetProperty(volumeProperty)

ren1.AddVolume(volume)

# ren1 AddActor contourActor
ren1.AddActor(boundsActor)

######################################################################
Sphere = vtk.vtkSphereSource()
Sphere.SetCenter(0, 0, 0)
Sphere.SetRadius(1)
Sphere.SetThetaResolution(16)
Sphere.SetStartTheta(0)
Sphere.SetEndTheta(360)
Sphere.SetPhiResolution(16)
Sphere.SetStartPhi(0)
Sphere.SetEndPhi(180)

Glyph = vtk.vtkGlyph3D()
Glyph.SetInputConnection(reader.GetOutputPort())
Glyph.SetOrient(1)
Glyph.SetColorMode(1)
# Glyph.ScalingOn()
Glyph.SetScaleMode(2)
Glyph.SetScaleFactor(.6)
Glyph.SetSourceConnection(Sphere.GetOutputPort())

AtomsMapper = vtk.vtkPolyDataMapper()
AtomsMapper.SetInputConnection(Glyph.GetOutputPort())
AtomsMapper.SetImmediateModeRendering(1)
AtomsMapper.UseLookupTableScalarRangeOff()
AtomsMapper.SetScalarVisibility(1)
AtomsMapper.SetScalarModeToDefault()

Atoms = vtk.vtkActor()
Atoms.SetMapper(AtomsMapper)
Atoms.GetProperty().SetRepresentationToSurface()
Atoms.GetProperty().SetInterpolationToGouraud()
Atoms.GetProperty().SetAmbient(0.15)
Atoms.GetProperty().SetDiffuse(0.85)
Atoms.GetProperty().SetSpecular(0.1)
Atoms.GetProperty().SetSpecularPower(100)
Atoms.GetProperty().SetSpecularColor(1, 1, 1)
Atoms.GetProperty().SetColor(1, 1, 1)

Tube = vtk.vtkTubeFilter()
Tube.SetInputConnection(reader.GetOutputPort())
Tube.SetNumberOfSides(16)
Tube.SetCapping(0)
Tube.SetRadius(0.2)
Tube.SetVaryRadius(0)
Tube.SetRadiusFactor(10)

BondsMapper = vtk.vtkPolyDataMapper()
BondsMapper.SetInputConnection(Tube.GetOutputPort())
BondsMapper.SetImmediateModeRendering(1)
BondsMapper.UseLookupTableScalarRangeOff()
BondsMapper.SetScalarVisibility(1)
BondsMapper.SetScalarModeToDefault()

Bonds = vtk.vtkActor()
Bonds.SetMapper(BondsMapper)
Bonds.GetProperty().SetRepresentationToSurface()
Bonds.GetProperty().SetInterpolationToGouraud()
Bonds.GetProperty().SetAmbient(0.15)
Bonds.GetProperty().SetDiffuse(0.85)
Bonds.GetProperty().SetSpecular(0.1)
Bonds.GetProperty().SetSpecularPower(100)
Bonds.GetProperty().SetSpecularColor(1, 1, 1)
Bonds.GetProperty().SetColor(1, 1, 1
                             )
ren1.AddActor(Bonds)
ren1.AddActor(Atoms)
####################################################
ren1.SetBackground(1, 1, 1)
ren1.ResetCamera()

renWin.Render()

def TkCheckAbort (object_binding, event_name):
    foo = renWin.GetEventPending()
    if (foo != 0):
        renWin.SetAbortRender(1)

renWin.AddObserver("AbortCheckEvent", TkCheckAbort)

iren.Initialize()
#iren.Start()
