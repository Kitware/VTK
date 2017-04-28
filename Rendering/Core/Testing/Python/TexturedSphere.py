#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

#
# Texture a sphere.
#

# renderer and interactor
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# read the volume
reader = vtk.vtkJPEGReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/beach.jpg")

#---------------------------------------------------------
# Do the surface rendering
sphereSource = vtk.vtkSphereSource()
sphereSource.SetRadius(100)

textureSphere = vtk.vtkTextureMapToSphere()
textureSphere.SetInputConnection(sphereSource.GetOutputPort())

sphereStripper = vtk.vtkStripper()
sphereStripper.SetInputConnection(textureSphere.GetOutputPort())
sphereStripper.SetMaximumLength(5)

sphereMapper = vtk.vtkPolyDataMapper()
sphereMapper.SetInputConnection(sphereStripper.GetOutputPort())
sphereMapper.ScalarVisibilityOff()

sphereTexture = vtk.vtkTexture()
sphereTexture.SetInputConnection(reader.GetOutputPort())
sphereProperty = vtk.vtkProperty()
# sphereProperty.BackfaceCullingOn()

sphere = vtk.vtkActor()
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
def TkCheckAbort (object_binding, event_name):
    foo = renWin.GetEventPending()
    if (foo != 0):
        renWin.SetAbortRender(1)

iren.Initialize()
#iren.Start()
