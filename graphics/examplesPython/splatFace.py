#!/usr/local/bin/python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

# splat points to generate surface

# Create the RenderWindow, Renderer and both Actors
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# read cyberware file
#
cyber = vtkPolyDataReader()
cyber.SetFileName(VTK_DATA + "/fran_cut.vtk")
normals = vtkPolyDataNormals()
normals.SetInput(cyber.GetOutput())
mask = vtkMaskPoints()
mask.SetInput(normals.GetOutput())
mask.SetOnRatio(50)
#    mask RandomModeOn
splatter = vtkGaussianSplatter()
splatter.SetInput(mask.GetOutput())
splatter.SetSampleDimensions(100,100,100)
splatter.SetEccentricity(2.5)
splatter.NormalWarpingOn()
splatter.SetScaleFactor(1.0)
splatter.SetRadius(0.025)
contour = vtkContourFilter()
contour.SetInput(splatter.GetOutput())
contour.SetValue(0,0.25)
splatMapper = vtkPolyDataMapper()
splatMapper.SetInput(contour.GetOutput())
splatMapper.ScalarVisibilityOff()
splatActor = vtkActor()
splatActor.SetMapper(splatMapper)
splatActor.GetProperty().SetColor(1.0,0.49,0.25)

cyberMapper = vtkPolyDataMapper()
cyberMapper.SetInput(cyber.GetOutput())
cyberMapper.ScalarVisibilityOff()
cyberActor = vtkActor()
cyberActor.SetMapper(cyberMapper)
cyberActor.GetProperty().SetRepresentationToWireframe()
cyberActor.GetProperty().SetColor(0.2510,0.8784,0.8157)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(cyberActor)
ren.AddActor(splatActor)
ren.SetBackground(1,1,1)
renWin.SetSize(500,500)
ren.SetBackground(1,1,1)
camera = vtkCamera()
camera.SetClippingRange(0.0332682,1.66341)
camera.SetFocalPoint(0.0511519,-0.127555,-0.0554379)
camera.SetPosition(0.516567,-0.124763,-0.349538)
camera.SetViewAngle(18.1279)
camera.SetViewUp(-0.013125,0.99985,-0.0112779)
ren.SetActiveCamera(camera)

# render the image
#
renWin.Render()


iren.Start()
