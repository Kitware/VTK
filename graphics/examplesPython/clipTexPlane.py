#!/usr/local/bin/python

from libVTKCommonPython import *
from libVTKGraphicsPython import *
from libVTKImagingPython import *

#
# clip a textured plane
#

#catch  load vtktcl 

# get the interactor ui
#source ../../examplesTcl/vtkInt.tcl

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

clipper = vtkClipPolyData()
clipper.GenerateClipScalarsOn()
clipper.SetValue(0)
clipper.SetClipFunction(clipPlane1)
clipper.SetInput(transformPlane.GetOutput())

planeMapper = vtkPolyDataMapper()
planeMapper.SetInput(clipper.GetOutput())
planeMapper.ScalarVisibilityOff()

planeActor = vtkActor()
planeActor.SetMapper(planeMapper)

plane2Mapper = vtkPolyDataMapper()
plane2Mapper.SetInput(plane.GetOutput())

plane2Actor = vtkActor()
plane2Actor.SetMapper(plane2Mapper)


# load in the texture map
#
atext = vtkTexture()
pnmReader = vtkPNMReader()
pnmReader.SetFileName("../../../vtkdata/masonry.ppm")

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

#renWin SetFileName "clipTexPlane.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
#wm withdraw .





iren.Start()
