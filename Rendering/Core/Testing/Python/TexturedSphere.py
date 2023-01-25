#!/usr/bin/env python
from vtkmodules.vtkFiltersCore import vtkStripper
from vtkmodules.vtkFiltersSources import vtkSphereSource
from vtkmodules.vtkFiltersTexture import vtkTextureMapToSphere
from vtkmodules.vtkIOImage import vtkJPEGReader
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
    vtkProperty,
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

#
# Texture a sphere.
#

# renderer and interactor
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# read the volume
reader = vtkJPEGReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/beach.jpg")

#---------------------------------------------------------
# Do the surface rendering
sphereSource = vtkSphereSource()
sphereSource.SetRadius(100)

textureSphere = vtkTextureMapToSphere()
textureSphere.SetInputConnection(sphereSource.GetOutputPort())

sphereStripper = vtkStripper()
sphereStripper.SetInputConnection(textureSphere.GetOutputPort())
sphereStripper.SetMaximumLength(5)

sphereMapper = vtkPolyDataMapper()
sphereMapper.SetInputConnection(sphereStripper.GetOutputPort())
sphereMapper.ScalarVisibilityOff()

sphereTexture = vtkTexture()
sphereTexture.SetInputConnection(reader.GetOutputPort())
sphereProperty = vtkProperty()
# sphereProperty.BackfaceCullingOn()

sphere = vtkActor()
sphere.SetMapper(sphereMapper)
sphere.SetTexture(sphereTexture)
sphere.SetProperty(sphereProperty)

#---------------------------------------------------------
ren.AddViewProp(sphere)

camera = ren.GetActiveCamera()
camera.SetFocalPoint(0, 0, 0)
camera.SetPosition(100, 400, -100)
camera.SetViewUp(0, 0, -1)

ren.ResetCameraClippingRange()
renWin.Render()
#---------------------------------------------------------
# test-related code
def TkCheckAbort(obj=None, event=""):
    if renWin.GetEventPending():
        renWin.SetAbortRender(1)

iren.Initialize()
#iren.Start()
