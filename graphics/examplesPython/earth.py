#!/usr/local/bin/python

from libVTKCommonPython import *
from libVTKGraphicsPython import *
from libVTKImagingPython import *
from libVTKContribPython import *

#catch  load vtktcl 
# this is a tcl version of the Mace example
# get the interactor ui
#source ../../examplesTcl/vtkInt.tcl

# Create the RenderWindow, Renderer and both Actors
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

tss = vtkTexturedSphereSource()
tss.SetThetaResolution(18)
tss.SetPhiResolution(9)
earthMapper = vtkPolyDataMapper()
earthMapper.SetInput(tss.GetOutput())
earthActor = vtkActor()
earthActor.SetMapper(earthMapper)

# load in the texture map
#
atext = vtkTexture()
pnmReader = vtkPNMReader()
pnmReader.SetFileName("../../../vtkdata/earth.ppm")
atext.SetInput(pnmReader.GetOutput())
atext.InterpolateOn()
earthActor.SetTexture(atext)

# create a earth source and actor
#
es = vtkEarthSource()
es.SetRadius(0.501)
es.SetOnRatio(2)
earth2Mapper = vtkPolyDataMapper()
earth2Mapper.SetInput(es.GetOutput())
earth2Actor = vtkActor()
earth2Actor.SetMapper(earth2Mapper)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(earthActor)
ren.AddActor(earth2Actor)
ren.SetBackground(0,0,0.1)
renWin.SetSize(300,300)

# render the image
#
cam1=ren.GetActiveCamera()
cam1.Zoom(1.4)
iren.Initialize()
#renWin SetFileName "earth.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
#wm withdraw .


iren.Start()
