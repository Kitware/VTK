#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *
from libVTKImagingPython import *

#
# clip a textured plane
#



# Create the RenderWindow, Renderer and both Actors
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Create a plane source
plane = vtkPlaneSource()

aTransform = vtkTransform()
aTransform.RotateX(30)
aTransform.RotateY(30)

transformPlane = vtkTransformPolyDataFilter()
transformPlane.SetInput(plane.GetOutput())
transformPlane.SetTransform(aTransform)

clipPlane1 = vtkPlane()
clipPlane1.SetNormal(0,0,1)

planeMapper = vtkDataSetMapper()
planeMapper.SetInput(transformPlane.GetOutput())
planeMapper.AddClippingPlane(clipPlane1)

planeActor = vtkActor()
planeActor.SetMapper(planeMapper)

plane2Mapper = vtkDataSetMapper()
plane2Mapper.SetInput(plane.GetOutput())

plane2Actor = vtkActor()
plane2Actor.SetMapper(plane2Mapper)


# load in the texture map
#
atext = vtkTexture()
pnmReader = vtkPNMReader()
pnmReader.SetFileName(VTK_DATA + "/masonry.ppm")

atext.SetInput(pnmReader.GetOutput())
atext.InterpolateOn()
planeActor.SetTexture(atext)

# Add the actors to the renderer, set the background and size
ren.AddActor(planeActor)
ren.AddActor(plane2Actor)
ren.SetBackground(0.1,0.2,0.4)
renWin.SetSize(500,500)

# render the image
iren.Initialize()
cam1=ren.GetActiveCamera()
cam1.Elevation(-30)
cam1.Roll(-20)
renWin.Render()







iren.Start()
