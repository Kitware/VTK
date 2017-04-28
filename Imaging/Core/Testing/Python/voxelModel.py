#!/usr/bin/env python
import os
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Now create the RenderWindow, Renderer and Interactor
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

sphereModel = vtk.vtkSphereSource()
sphereModel.SetThetaResolution(10)
sphereModel.SetPhiResolution(10)

voxelModel = vtk.vtkVoxelModeller()
voxelModel.SetInputConnection(sphereModel.GetOutputPort())
voxelModel.SetSampleDimensions(21, 21, 21)
voxelModel.SetModelBounds(-1.5, 1.5, -1.5, 1.5, -1.5, 1.5)
voxelModel.SetScalarTypeToBit()
voxelModel.SetForegroundValue(1)
voxelModel.SetBackgroundValue(0)

#
# If the current directory is writable, then test the writers
#
try:
    channel = open("voxelModel.vtk", "wb")
    channel.close()

    aWriter = vtk.vtkDataSetWriter()
    aWriter.SetFileName("voxelModel.vtk")
    aWriter.SetInputConnection(voxelModel.GetOutputPort())
    aWriter.Update()

    aReader = vtk.vtkDataSetReader()
    aReader.SetFileName("voxelModel.vtk")

    voxelSurface = vtk.vtkContourFilter()
    voxelSurface.SetInputConnection(aReader.GetOutputPort())
    voxelSurface.SetValue(0, .999)

    voxelMapper = vtk.vtkPolyDataMapper()
    voxelMapper.SetInputConnection(voxelSurface.GetOutputPort())

    voxelActor = vtk.vtkActor()
    voxelActor.SetMapper(voxelMapper)

    sphereMapper = vtk.vtkPolyDataMapper()
    sphereMapper.SetInputConnection(sphereModel.GetOutputPort())

    sphereActor = vtk.vtkActor()
    sphereActor.SetMapper(sphereMapper)

    ren1.AddActor(sphereActor)
    ren1.AddActor(voxelActor)
    ren1.SetBackground(.1, .2, .4)

    renWin.SetSize(256, 256)

    ren1.ResetCamera()
    ren1.GetActiveCamera().SetViewUp(0, -1, 0)
    ren1.GetActiveCamera().Azimuth(180)
    ren1.GetActiveCamera().Dolly(1.75)
    ren1.ResetCameraClippingRange()

    # cleanup
    #
    try:
        os.remove("voxelModel.vtk")
    except OSError:
        pass

    iren.Initialize()
    # render the image
#    iren.Start()

except IOError:
    print("Unable to test the writer/reader.")
