#!/usr/local/bin/python

from libVTKCommonPython import *
from libVTKGraphicsPython import *

#catch  load vtktcl 
# user interface command widget
#source ../../examplesTcl/vtkInt.tcl

# create a rendering window and renderer
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
renWin.SetSize(300,300)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create an actor and give it cone geometry
cone = vtkConeSource()
cone.SetResolution(8)
coneMapper = vtkPolyDataMapper()
coneMapper.SetInput(cone.GetOutput())
coneActor = vtkActor()
coneActor.SetMapper(coneMapper)

# assign our actor to the renderer
ren.AddActor(coneActor)

# enable user interface interactor
iren.Initialize()

# prevent the tk window from showing up then start the event loop
#wm withdraw .

iren.Start()
