#!/usr/local/bin/python

from libVTKCommonPython import *
from libVTKGraphicsPython import *

#catch  load vtktcl 
# this is a tcl version of motor visualization
# get the interactor ui
#source ../../examplesTcl/vtkInt.tcl

# Create the RenderWindow, Renderer and both Actors
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# read a vtk file
#
byu = vtkBYUReader()
byu.SetGeometryFileName("../../../vtkdata/fohe.g")
normals = vtkPolyDataNormals()
normals.SetInput(byu.GetOutput())
byuMapper = vtkPolyDataMapper()
byuMapper.SetInput(normals.GetOutput())
byuActor = vtkActor()
byuActor.SetMapper(byuMapper)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(byuActor)
ren.SetBackground(0.2,0.3,0.4)
renWin.SetSize(500,500)

# render the image
#
iren.Initialize()

#renWin SetFileName "fohe.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
#wm withdraw .


iren.Start()
