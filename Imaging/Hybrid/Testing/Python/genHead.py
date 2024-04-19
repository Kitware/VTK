#!/usr/bin/env python
import os
from vtkmodules.vtkCommonColor import vtkNamedColors
from vtkmodules.vtkIOGeometry import vtkMCubesReader
from vtkmodules.vtkIOImage import vtkVolume16Reader
from vtkmodules.vtkImagingHybrid import vtkSliceCubes
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

def GetRGBColor(colorName):
    '''
        Return the red, green and blue components for a
        color as doubles.
    '''
    rgb = [0.0, 0.0, 0.0]  # black
    vtkNamedColors().GetColorRGB(colorName, rgb)
    return rgb

#
# The current directory must be writeable.
#
try:
    channel = open("fullHead.tri", "wb")
    channel.close()

    # reader reads slices
    v16 = vtkVolume16Reader()
    v16.SetDataDimensions(64, 64)
    v16.SetDataByteOrderToLittleEndian()
    v16.SetFilePrefix(VTK_DATA_ROOT + "/Data/headsq/quarter")
    v16.SetDataSpacing(3.2, 3.2, 1.5)
    v16.SetImageRange(30, 50)
    v16.SetDataMask(0x7fff)

    # write isosurface to file
    mcubes = vtkSliceCubes()
    mcubes.SetReader(v16)
    mcubes.SetValue(1150)
    mcubes.SetFileName("fullHead.tri")
    mcubes.SetLimitsFileName("fullHead.lim")
    mcubes.Update()

    # read from file
    reader = vtkMCubesReader()
    reader.SetFileName("fullHead.tri")
    reader.SetLimitsFileName("fullHead.lim")
    mapper = vtkPolyDataMapper()
    mapper.SetInputConnection(reader.GetOutputPort())
    head = vtkActor()
    head.SetMapper(mapper)
    head.GetProperty().SetColor(GetRGBColor('raw_sienna'))

    # Create the RenderWindow, Renderer and Interactor
    #
    ren1 = vtkRenderer()
    renWin = vtkRenderWindow()
    renWin.AddRenderer(ren1)
    iren = vtkRenderWindowInteractor()
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
    print("Unable to test the writer/reader.")
