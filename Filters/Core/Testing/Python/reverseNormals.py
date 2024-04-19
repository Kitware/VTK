#!/usr/bin/env python
from vtkmodules.vtkCommonColor import vtkNamedColors
from vtkmodules.vtkCommonDataModel import vtkPlane
from vtkmodules.vtkCommonTransforms import vtkTransform
from vtkmodules.vtkFiltersCore import (
    vtkClipPolyData,
    vtkPolyDataNormals,
    vtkReverseSense,
)
from vtkmodules.vtkFiltersGeneral import vtkTransformPolyDataFilter
from vtkmodules.vtkIOGeometry import vtkOBJReader
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

# Now create the RenderWindow, Renderer and Interactor
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

cowReader = vtkOBJReader()
cowReader.SetFileName(VTK_DATA_ROOT + "/Data/Viewpoint/cow.obj")

plane = vtkPlane()
plane.SetNormal(1, 0, 0)

cowClipper = vtkClipPolyData()
cowClipper.SetInputConnection(cowReader.GetOutputPort())
cowClipper.SetClipFunction(plane)

cellNormals = vtkPolyDataNormals()
cellNormals.SetInputConnection(cowClipper.GetOutputPort())
cellNormals.ComputePointNormalsOn()
cellNormals.ComputeCellNormalsOn()

reflect = vtkTransform()
reflect.Scale(-1, 1, 1)

cowReflect = vtkTransformPolyDataFilter()
cowReflect.SetTransform(reflect)
cowReflect.SetInputConnection(cellNormals.GetOutputPort())

cowReverse = vtkReverseSense()
cowReverse.SetInputConnection(cowReflect.GetOutputPort())
cowReverse.ReverseNormalsOn()
cowReverse.ReverseCellsOff()

reflectedMapper = vtkPolyDataMapper()
reflectedMapper.SetInputConnection(cowReverse.GetOutputPort())

reflected = vtkActor()
reflected.SetMapper(reflectedMapper)
reflected.GetProperty().SetDiffuseColor(GetRGBColor('flesh'))
reflected.GetProperty().SetDiffuse(.8)
reflected.GetProperty().SetSpecular(.5)
reflected.GetProperty().SetSpecularPower(30)
reflected.GetProperty().FrontfaceCullingOn()

ren1.AddActor(reflected)

cowMapper = vtkPolyDataMapper()
cowMapper.SetInputConnection(cowClipper.GetOutputPort())

cow = vtkActor()
cow.SetMapper(cowMapper)

ren1.AddActor(cow)

ren1.SetBackground(.1, .2, .4)

renWin.SetSize(320, 240)

ren1.ResetCamera()
ren1.GetActiveCamera().SetViewUp(0, 1, 0)
ren1.GetActiveCamera().Azimuth(180)
ren1.GetActiveCamera().Dolly(1.75)
ren1.ResetCameraClippingRange()

iren.Initialize()

# render the image
#iren.Start()
