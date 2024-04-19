#!/usr/bin/env python
from vtkmodules.vtkCommonColor import vtkNamedColors
from vtkmodules.vtkFiltersSources import (
    vtkCubeSource,
    vtkSphereSource,
)
from vtkmodules.vtkFiltersTexture import vtkTriangularTCoords
from vtkmodules.vtkImagingHybrid import vtkTriangularTexture
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
    vtkTexture,
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

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

aTriangularTexture = vtkTriangularTexture()
aTriangularTexture.SetTexturePattern(2)
aTriangularTexture.SetScaleFactor(1.3)
aTriangularTexture.SetXSize(64)
aTriangularTexture.SetYSize(64)

aSphere = vtkSphereSource()
aSphere.SetThetaResolution(20)
aSphere.SetPhiResolution(20)

tCoords = vtkTriangularTCoords()
tCoords.SetInputConnection(aSphere.GetOutputPort())

triangleMapper = vtkPolyDataMapper()
triangleMapper.SetInputConnection(tCoords.GetOutputPort())

aTexture = vtkTexture()
aTexture.SetInputConnection(aTriangularTexture.GetOutputPort())
aTexture.InterpolateOn()

texturedActor = vtkActor()
texturedActor.SetMapper(triangleMapper)
texturedActor.SetTexture(aTexture)
texturedActor.GetProperty().BackfaceCullingOn()
texturedActor.GetProperty().SetDiffuseColor(GetRGBColor('banana'))
texturedActor.GetProperty().SetSpecular(.4)
texturedActor.GetProperty().SetSpecularPower(40)

aCube = vtkCubeSource()
aCube.SetXLength(.5)
aCube.SetYLength(.5)

aCubeMapper = vtkPolyDataMapper()
aCubeMapper.SetInputConnection(aCube.GetOutputPort())

cubeActor = vtkActor()
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
