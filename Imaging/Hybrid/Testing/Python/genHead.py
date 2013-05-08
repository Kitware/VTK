#!/usr/bin/env python
import os
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

def GetRGBColor(colorName):
    '''
        Return the red, green and blue components for a
        color as doubles.
    '''
    rgb = [0.0, 0.0, 0.0]  # black
    vtk.vtkNamedColors().GetColorRGB(colorName, rgb)
    return rgb

#
# The current directory must be writeable.
#
try:
    channel = open("fullHead.tri", "wb")
    channel.close()

    # reader reads slices
    v16 = vtk.vtkVolume16Reader()
    v16.SetDataDimensions(64, 64)
    v16.SetDataByteOrderToLittleEndian()
    v16.SetFilePrefix(VTK_DATA_ROOT + "/Data/headsq/quarter")
    v16.SetDataSpacing(3.2, 3.2, 1.5)
    v16.SetImageRange(30, 50)
    v16.SetDataMask(0x7fff)

    # write isosurface to file
    mcubes = vtk.vtkSliceCubes()
    mcubes.SetReader(v16)
    mcubes.SetValue(1150)
    mcubes.SetFileName("fullHead.tri")
    mcubes.SetLimitsFileName("fullHead.lim")
    mcubes.Update()

    # read from file
    reader = vtk.vtkMCubesReader()
    reader.SetFileName("fullHead.tri")
    reader.SetLimitsFileName("fullHead.lim")
    mapper = vtk.vtkPolyDataMapper()
    mapper.SetInputConnection(reader.GetOutputPort())
    head = vtk.vtkActor()
    head.SetMapper(mapper)
    head.GetProperty().SetColor(GetRGBColor('raw_sienna'))

    # Create the RenderWindow, Renderer and Interactor
    #
    ren1 = vtk.vtkRenderer()
    renWin = vtk.vtkRenderWindow()
    renWin.AddRenderer(ren1)
    iren = vtk.vtkRenderWindowInteractor()
    iren.SetRenderWindow(renWin)

    # Add the actors to the renderer, set the background and size
    #
    ren1.AddActor(head)
    ren1.SetBackground(1, 1, 1)

    renWin.SetSize(500, 500)

    ren1.SetBackground(GetRGBColor('slate_grey'))

    ren1.GetActiveCamera().SetPosition(99.8847, 537.926, 15)
    ren1.GetActiveCamera().SetFocalPoint(99.8847, 109.81, 15)
    ren1.GetActiveCamera().SetViewAngle(20)
    ren1.GetActiveCamera().SetViewUp(0, 0, -1)
    ren1.ResetCameraClippingRange()

    # cleanup
    #
    try:
        os.remove("fullHead.tri")
    except OSError:
        pass
    try:
        os.remove("fullHead.lim")
    except OSError:
        pass

    # render the image
    #
    renWin.Render()

    iren.Initialize()
    # render the image
#    iren.Start()

except IOError:
    print  "Unable to test the writer/reader."
