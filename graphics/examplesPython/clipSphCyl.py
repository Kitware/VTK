#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

#
# Demonstrate the use of clipping on polygonal data
#
from colors import *
# create pipeline
#
plane = vtkPlaneSource()
plane.SetXResolution(25)
plane.SetYResolution(25)
plane.SetOrigin(-1,-1,0)
plane.SetPoint1(1,-1,0)
plane.SetPoint2(-1,1,0)

transformSphere = vtkTransform()
transformSphere.Identity()
transformSphere.Translate(.4,-.4,0)
transformSphere.Inverse()

sphere = vtkSphere()
sphere.SetTransform(transformSphere)
sphere.SetRadius(.5)

transformCylinder = vtkTransform()
transformCylinder.Identity()
transformCylinder.Translate(-.4,.4,0)
transformCylinder.RotateZ(30)
transformCylinder.RotateY(60)
transformCylinder.RotateX(90)
transformCylinder.Inverse()

cylinder = vtkCylinder()
cylinder.SetTransform(transformCylinder)
cylinder.SetRadius(.3)

boolean = vtkImplicitBoolean()
boolean.AddFunction(cylinder)
boolean.AddFunction(sphere)

clipper = vtkClipPolyData()
clipper.SetInput(plane.GetOutput())
clipper.SetClipFunction(boolean)
clipper.GenerateClippedOutputOn()
clipper.GenerateClipScalarsOn()
clipper.SetValue(0)

clipMapper = vtkPolyDataMapper()
clipMapper.SetInput(clipper.GetOutput())
clipMapper.ScalarVisibilityOff()

clipActor = vtkActor()
clipActor.SetMapper(clipMapper)
clipActor.GetProperty().SetDiffuseColor(black)
clipActor.GetProperty().SetRepresentationToWireframe()

clipInsideMapper = vtkPolyDataMapper()
clipInsideMapper.SetInput(clipper.GetClippedOutput())
clipInsideMapper.ScalarVisibilityOff()
clipInsideActor = vtkActor()
clipInsideActor.SetMapper(clipInsideMapper)
clipInsideActor.GetProperty().SetDiffuseColor(dim_grey)

# Create graphics stuff
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(clipActor)
#[clipActor GetProperty] SetWireframe

ren.AddActor(clipInsideActor)
ren.SetBackground(1,1,1)
ren.GetActiveCamera().Dolly(1.5)
ren.ResetCameraClippingRange()

renWin.SetSize(512,512)
iren.Initialize()

# render the image
#




iren.Start()
