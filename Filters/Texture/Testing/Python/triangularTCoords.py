#!/usr/bin/env python
import vtk
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

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

aTriangularTexture = vtk.vtkTriangularTexture()
aTriangularTexture.SetTexturePattern(2)
aTriangularTexture.SetScaleFactor(1.3)
aTriangularTexture.SetXSize(64)
aTriangularTexture.SetYSize(64)

aSphere = vtk.vtkSphereSource()
aSphere.SetThetaResolution(20)
aSphere.SetPhiResolution(20)

tCoords = vtk.vtkTriangularTCoords()
tCoords.SetInputConnection(aSphere.GetOutputPort())

triangleMapper = vtk.vtkPolyDataMapper()
triangleMapper.SetInputConnection(tCoords.GetOutputPort())

aTexture = vtk.vtkTexture()
aTexture.SetInputConnection(aTriangularTexture.GetOutputPort())
aTexture.InterpolateOn()

texturedActor = vtk.vtkActor()
texturedActor.SetMapper(triangleMapper)
texturedActor.SetTexture(aTexture)
texturedActor.GetProperty().BackfaceCullingOn()
texturedActor.GetProperty().SetDiffuseColor(GetRGBColor('banana'))
texturedActor.GetProperty().SetSpecular(.4)
texturedActor.GetProperty().SetSpecularPower(40)

aCube = vtk.vtkCubeSource()
aCube.SetXLength(.5)
aCube.SetYLength(.5)

aCubeMapper = vtk.vtkPolyDataMapper()
aCubeMapper.SetInputConnection(aCube.GetOutputPort())

cubeActor = vtk.vtkActor()
cubeActor.SetMapper(aCubeMapper)
cubeActor.GetProperty().SetDiffuseColor(GetRGBColor('tomato'))

ren1.SetBackground(GetRGBColor('slate_grey'))
ren1.AddActor(cubeActor)
ren1.AddActor(texturedActor)

ren1.ResetCamera()

ren1.GetActiveCamera().Zoom(1.5)

# render the image
#
iren.Initialize()
#iren.Start()
