#!/usr/local/bin/python

from libVTKCommonPython import *
from libVTKGraphicsPython import *
from libVTKImagingPython import *

#catch  load vtktcl 
# Test the texture transformation object

# Get the interactor
#source ../../examplesTcl/vtkInt.tcl

# load in the texture map
#
pnmReader = vtkPNMReader()
pnmReader.SetFileName("../../../vtkdata/masonry.ppm")
atext = vtkTexture()
atext.SetInput(pnmReader.GetOutput())
atext.InterpolateOn()

# create a plane source and actor
plane = vtkPlaneSource()
trans = vtkTransformTextureCoords()
trans.SetInput(plane.GetOutput())
trans.SetScale(2,3,1)
trans.FlipSOn()
trans.SetPosition(0.5,1.0,0.0) #need.to.do.this.because.of.non-zero.origin
planeMapper = vtkDataSetMapper()
planeMapper.SetInput(trans.GetOutput())
planeActor = vtkActor()
planeActor.SetMapper(planeMapper)
planeActor.SetTexture(atext)

# Create graphics stuff
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
ren.AddActor(planeActor)
ren.SetBackground(0.1,0.2,0.4)
renWin.SetSize(500,500)
renWin.Render()

#renWin SetFileName "texTrans.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
#wm withdraw .





iren.Start()
