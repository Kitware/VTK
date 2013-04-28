#!/usr/bin/env python
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

# Now create the RenderWindow, Renderer and Interactor
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

cowReader = vtk.vtkOBJReader()
cowReader.SetFileName(VTK_DATA_ROOT + "/Data/Viewpoint/cow.obj")

plane = vtk.vtkPlane()
plane.SetNormal(1, 0, 0)

cowClipper = vtk.vtkClipPolyData()
cowClipper.SetInputConnection(cowReader.GetOutputPort())
cowClipper.SetClipFunction(plane)

cellNormals = vtk.vtkPolyDataNormals()
cellNormals.SetInputConnection(cowClipper.GetOutputPort())
cellNormals.ComputePointNormalsOn()
cellNormals.ComputeCellNormalsOn()

reflect = vtk.vtkTransform()
reflect.Scale(-1, 1, 1)

cowReflect = vtk.vtkTransformPolyDataFilter()
cowReflect.SetTransform(reflect)
cowReflect.SetInputConnection(cellNormals.GetOutputPort())

cowReverse = vtk.vtkReverseSense()
cowReverse.SetInputConnection(cowReflect.GetOutputPort())
cowReverse.ReverseNormalsOn()
cowReverse.ReverseCellsOff()

reflectedMapper = vtk.vtkPolyDataMapper()
reflectedMapper.SetInputConnection(cowReverse.GetOutputPort())

reflected = vtk.vtkActor()
reflected.SetMapper(reflectedMapper)
reflected.GetProperty().SetDiffuseColor(GetRGBColor('flesh'))
reflected.GetProperty().SetDiffuse(.8)
reflected.GetProperty().SetSpecular(.5)
reflected.GetProperty().SetSpecularPower(30)
reflected.GetProperty().FrontfaceCullingOn()

ren1.AddActor(reflected)

cowMapper = vtk.vtkPolyDataMapper()
cowMapper.SetInputConnection(cowClipper.GetOutputPort())

cow = vtk.vtkActor()
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
