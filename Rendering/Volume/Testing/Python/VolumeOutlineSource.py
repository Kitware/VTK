#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Simple volume rendering example.
reader = vtk.vtkSLCReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/sphere.slc")

# Create transfer functions for opacity and color
opacityTransferFunction = vtk.vtkPiecewiseFunction()
opacityTransferFunction.AddPoint(0, 0.0)
opacityTransferFunction.AddPoint(30, 0.0)
opacityTransferFunction.AddPoint(80, 0.5)
opacityTransferFunction.AddPoint(255, 0.5)

colorTransferFunction = vtk.vtkColorTransferFunction()
colorTransferFunction.AddRGBPoint(0.0, 0.0, 0.0, 0.0)
colorTransferFunction.AddRGBPoint(64.0, 1.0, 0.0, 0.0)
colorTransferFunction.AddRGBPoint(128.0, 0.0, 0.0, 1.0)
colorTransferFunction.AddRGBPoint(192.0, 0.0, 1.0, 0.0)
colorTransferFunction.AddRGBPoint(255.0, 0.0, 0.2, 0.0)

# Create properties, mappers, volume actors, and ray cast function
volumeProperty = vtk.vtkVolumeProperty()
volumeProperty.SetColor(colorTransferFunction)
volumeProperty.SetScalarOpacity(opacityTransferFunction)
volumeProperty.SetInterpolationTypeToLinear()
volumeProperty.ShadeOn()

ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)

renWin.SetSize(600, 300)

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

renWin.SetMultiSamples(0)
ren1.SetBackground(0.1, 0.2, 0.4)

i = 0
while i < 2:
    j = 0
    while j < 4:
        idx = str(i) + "_" + str(j)

        exec("volumeMapper_" + idx + " = vtk.vtkVolumeTextureMapper2D()")
        eval("volumeMapper_" + idx).SetInputConnection(reader.GetOutputPort())
        eval("volumeMapper_" + idx).CroppingOn()
        eval("volumeMapper_" + idx).SetCroppingRegionPlanes(
          17.5, 32.5, 17.5, 32.5, 17.5, 32.5)

        exec("volume_" + idx + " = vtk.vtkVolume()")
        eval("volume_" + idx).SetMapper(eval("volumeMapper_" + idx))
        eval("volume_" + idx).SetProperty(volumeProperty)

        exec("outline_" + idx + " = vtk.vtkVolumeOutlineSource()")
        eval("outline_" + idx).SetVolumeMapper(eval("volumeMapper_" + idx))

        exec("outlineMapper_" + idx + " = vtk.vtkDataSetMapper()")
        eval("outlineMapper_" + idx).SetInputConnection(
          eval("outline_" + idx).GetOutputPort())

        exec("outlineActor_" + idx + " = vtk.vtkActor()")
        eval("outlineActor_" + idx).SetMapper(eval("outlineMapper_" + idx))

        exec("userMatrix_" + idx + " = vtk.vtkTransform()")
        eval("userMatrix_" + idx).PostMultiply()
        eval("userMatrix_" + idx).Identity()
        eval("userMatrix_" + idx).Translate(-25, -25, -25)
        if (i == 0):
            eval("userMatrix_" + idx).RotateX(j * 87 + 23)
            eval("userMatrix_" + idx).RotateY(16)
        else:
            eval("userMatrix_" + idx).RotateX(27)
            eval("userMatrix_" + idx).RotateY(j * 87 + 19)
        eval("userMatrix_" + idx).Translate(j * 65 + 25, i * 55 + 25, 0)

        eval("volume_" + idx).SetUserTransform(eval("userMatrix_" + idx))

        eval("outlineActor_" + idx).SetUserTransform(eval("userMatrix_" + idx))

        ren1.AddViewProp(eval("outlineActor_" + idx))
        ren1.AddViewProp(eval("volume_" + idx))

        j = j + 1

    i = i + 1

volumeMapper_0_0.SetCroppingRegionFlagsToSubVolume()
volumeMapper_0_1.SetCroppingRegionFlagsToCross()
volumeMapper_0_2.SetCroppingRegionFlagsToInvertedCross()
volumeMapper_0_3.SetCroppingRegionFlags(24600)
volumeMapper_1_0.SetCroppingRegionFlagsToFence()
volumeMapper_1_1.SetCroppingRegionFlagsToInvertedFence()
volumeMapper_1_2.SetCroppingRegionFlags(1)
volumeMapper_1_3.SetCroppingRegionFlags(67117057)

outline_0_1.GenerateScalarsOn()

outline_0_2.GenerateScalarsOn()
outline_0_2.SetActivePlaneId(1)
outline_0_2.SetColor(1, 0, 1)

outline_0_3.GenerateScalarsOn()
outline_0_3.SetActivePlaneId(2)
outline_0_3.SetActivePlaneColor(0, 1, 1)

outlineActor_1_0.GetProperty().SetColor(0, 1, 0)

outline_1_1.GenerateFacesOn()
volume_1_1.VisibilityOff()

outline_1_2.GenerateFacesOn()
outline_1_2.GenerateScalarsOn()

volume_1_2.VisibilityOff()

outline_1_3.GenerateFacesOn()
outline_1_3.GenerateScalarsOn()
outline_1_3.SetActivePlaneId(1)

volume_1_3.VisibilityOff()

ren1.GetCullers().InitTraversal()

culler = ren1.GetCullers().GetNextItem()
culler.SetSortingStyleToBackToFront()

ren1.ResetCamera()
ren1.GetActiveCamera().Zoom(2.35)
renWin.Render()

def TkCheckAbort (object_binding, event_name):
    foo = renWin.GetEventPending()
    if (foo != 0):
        renWin.SetAbortRender(1)

renWin.AddObserver("AbortCheckEvent", TkCheckAbort)

iren.Initialize()
#iren.Start()
