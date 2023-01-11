#!/usr/bin/env python
import os
from vtkmodules.vtkFiltersCore import vtkContourFilter
from vtkmodules.vtkFiltersSources import vtkSphereSource
from vtkmodules.vtkIOLegacy import (
    vtkDataSetReader,
    vtkDataSetWriter,
)
from vtkmodules.vtkImagingHybrid import vtkVoxelModeller
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
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

sphereModel = vtkSphereSource()
sphereModel.SetThetaResolution(10)
sphereModel.SetPhiResolution(10)

voxelModel = vtkVoxelModeller()
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

    aWriter = vtkDataSetWriter()
    aWriter.SetFileName("voxelModel.vtk")
    aWriter.SetInputConnection(voxelModel.GetOutputPort())
    aWriter.Update()

    aReader = vtkDataSetReader()
    aReader.SetFileName("voxelModel.vtk")

    voxelSurface = vtkContourFilter()
    voxelSurface.SetInputConnection(aReader.GetOutputPort())
    voxelSurface.SetValue(0, .999)

    voxelMapper = vtkPolyDataMapper()
    voxelMapper.SetInputConnection(voxelSurface.GetOutputPort())

    voxelActor = vtkActor()
    voxelActor.SetMapper(voxelMapper)

    sphereMapper = vtkPolyDataMapper()
    sphereMapper.SetInputConnection(sphereModel.GetOutputPort())

    sphereActor = vtkActor()
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
