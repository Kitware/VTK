#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

# showing diffs between flat & gouraud

ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create a sphere source and actor
#
sphere = vtkSphereSource()
sphere.SetThetaResolution(30)
sphere.SetPhiResolution(30)
sphereMapper = vtkPolyDataMapper()
sphereMapper.SetInput(sphere.GetOutput())
sphereActor = vtkLODActor()
sphereActor.SetMapper(sphereMapper)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(sphereActor)
ren.SetBackground(0.1,0.2,0.4)
renWin.SetSize(375,375)

# render the image
#
cam1=ren.GetActiveCamera()
cam1.Zoom(1.4)
iren.Initialize()
cam1.Azimuth(30)
cam1.Elevation(-50)

prop=sphereActor.GetProperty()
prop.SetDiffuseColor(1.0,0,0)
prop.SetDiffuse(0.6)
prop.SetSpecularPower(5)
prop.SetSpecular(0.5)
renWin.Render()
renWin.SetFileName("f1.ppm")

prop.SetSpecular(1.0)
renWin.Render()




iren.Start()
