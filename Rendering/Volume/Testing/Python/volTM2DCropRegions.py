#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Simple volume rendering example.
reader = vtk.vtkSLCReader()
reader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/sphere.slc")
# Create transfer functions for opacity and color
opacityTransferFunction = vtk.vtkPiecewiseFunction()
opacityTransferFunction.AddPoint(0,0.0)
opacityTransferFunction.AddPoint(30,0.0)
opacityTransferFunction.AddPoint(80,0.5)
opacityTransferFunction.AddPoint(255,0.5)
colorTransferFunction = vtk.vtkColorTransferFunction()
colorTransferFunction.AddRGBPoint(0.0,0.0,0.0,0.0)
colorTransferFunction.AddRGBPoint(64.0,1.0,0.0,0.0)
colorTransferFunction.AddRGBPoint(128.0,0.0,0.0,1.0)
colorTransferFunction.AddRGBPoint(192.0,0.0,1.0,0.0)
colorTransferFunction.AddRGBPoint(255.0,0.0,0.2,0.0)
# Create properties, mappers, volume actors, and ray cast function
volumeProperty = vtk.vtkVolumeProperty()
volumeProperty.SetColor(colorTransferFunction)
volumeProperty.SetScalarOpacity(opacityTransferFunction)
volumeProperty.SetInterpolationTypeToLinear()
volumeProperty.ShadeOn()
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.SetSize(600,300)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
ren1.SetBackground(0.1,0.2,0.4)
i = 0
while i < 2:
    j = 0
    while j < 4:
        locals()[get_variable_name("volumeMapper_", i, "_", j, "")] = vtk.vtkVolumeTextureMapper2D()
        locals()[get_variable_name("volumeMapper_", i, "_", j, "")].SetInputConnection(reader.GetOutputPort())
        locals()[get_variable_name("volumeMapper_", i, "_", j, "")].CroppingOn()
        locals()[get_variable_name("volumeMapper_", i, "_", j, "")].SetCroppingRegionPlanes(17,33,17,33,17,33)
        locals()[get_variable_name("volume_", i, "_", j, "")] = vtk.vtkVolume()
        locals()[get_variable_name("volume_", i, "_", j, "")].SetMapper(locals()[get_variable_name("volumeMapper_", i, "_", j, "")])
        locals()[get_variable_name("volume_", i, "_", j, "")].SetProperty(volumeProperty)
        locals()[get_variable_name("userMatrix_", i, "_", j, "")] = vtk.vtkTransform()
        locals()[get_variable_name("userMatrix_", i, "_", j, "")].PostMultiply()
        locals()[get_variable_name("userMatrix_", i, "_", j, "")].Identity()
        locals()[get_variable_name("userMatrix_", i, "_", j, "")].Translate(-25,-25,-25)
        if (i == 0):
            locals()[get_variable_name("userMatrix_", i, "_", j, "")].RotateX(expr.expr(globals(), locals(),["j","*","90","+","20"]))
            locals()[get_variable_name("userMatrix_", i, "_", j, "")].RotateY(20)
            pass
        else:
            locals()[get_variable_name("userMatrix_", i, "_", j, "")].RotateX(20)
            locals()[get_variable_name("userMatrix_", i, "_", j, "")].RotateY(expr.expr(globals(), locals(),["j","*","90","+","20"]))
            pass
        locals()[get_variable_name("userMatrix_", i, "_", j, "")].Translate(expr.expr(globals(), locals(),["j","*","55","+","25"]),expr.expr(globals(), locals(),["i","*","55","+","25"]),0)
        locals()[get_variable_name("volume_", i, "_", j, "")].SetUserTransform(locals()[get_variable_name("userMatrix_", i, "_", j, "")])
        ren1.AddViewProp(locals()[get_variable_name("volume_", i, "_", j, "")])
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
ren1.GetCullers().InitTraversal()
culler = ren1.GetCullers().GetNextItem()
culler.SetSortingStyleToBackToFront()
ren1.ResetCamera()
ren1.GetActiveCamera().Zoom(3.0)
renWin.Render()
def TkCheckAbort (__vtk__temp0=0,__vtk__temp1=0):
    foo = renWin.GetEventPending()
    if (foo != 0):
        renWin.SetAbortRender(1)
        pass

renWin.AddObserver("AbortCheckEvent",TkCheckAbort)
iren.Initialize()
# --- end of script --
