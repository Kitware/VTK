#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'
from libVTKGraphicsPython import *

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer( ren1 )
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow( renWin )

# disable old light-tracking behavior
iren.LightFollowCameraOff()

# create lights
#
# CameraLights are defined in a local coordinate system where
# the camera is at (0, 0, 1), looking at (0, 0, 0), with up
# being (0, 1, 0)
#
keyLight = vtkLight()
keyLight.SetLightTypeToCameraLight()
keyLight.SetPosition(0.25, 1.0, 0.75)
keyLight.SetColor(1.0, 1.0, 0.9)
ren1.AddLight(keyLight)

fillLight = vtkLight()
fillLight.SetLightTypeToCameraLight()
fillLight.SetPosition(-0.25, -1.0, 0.75)
fillLight.SetIntensity(0.2)
fillLight.SetColor(0.75, 0.75, 1)
ren1.AddLight(fillLight)

# A Headlight is always at the camera.
#
headlight = vtkLight()
headlight.SetLightTypeToHeadlight()
headlight.SetIntensity(0.1)
ren1.AddLight(headlight)

# create a sphere source and actor
#
sphere = vtkSphereSource()
sphereMapper = vtkPolyDataMapper()
sphereMapper.SetInput( sphere.GetOutput() )
sphereActor = vtkLODActor()
sphereActor.SetMapper( sphereMapper )

# create the spikes using a cone source and the sphere source
#
cone = vtkConeSource()

glyph = vtkGlyph3D()
glyph.SetInput( sphere.GetOutput() )
glyph.SetSource(cone.GetOutput())
glyph.SetVectorModeToUseNormal()
glyph.SetScaleModeToScaleByVector()
glyph.SetScaleFactor( 0.25 )
glyph.ReleaseDataFlagOn()

spikeMapper = vtkPolyDataMapper()
spikeMapper.SetInput( glyph.GetOutput() )
spikeActor = vtkLODActor()
spikeActor.SetMapper( spikeMapper )

# Add the actors to the renderer,.Set the background and size
#
ren1.AddActor( sphereActor )
ren1.AddActor( spikeActor )
ren1.SetBackground( 0.1, 0.2, 0.4 )
renWin.SetSize( 300, 300 )

# render the image
#
iren.Initialize()

iren.Start()

