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
renWin.SetSize(400,400)

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

cube = vtkConeSource()
#vtkPlaneSource cube
#  cube SetResolution 5 5
#vtkCubeSource cube
clean = vtkCleanPolyData()
clean.SetInput(cube.GetOutput())
extract = vtkExtractEdges()
extract.SetInput(clean.GetOutput())
tubes = vtkTubeFilter()
tubes.SetInput(extract.GetOutput())
tubes.SetRadius(0.05)
tubes.SetNumberOfSides(6)
mapper = vtkPolyDataMapper()
mapper.SetInput(tubes.GetOutput())
cubeActor = vtkActor()
cubeActor.SetMapper(mapper)

sphere = vtkSphereSource()
sphere.SetRadius(0.080)
verts = vtkGlyph3D()
verts.SetInput(cube.GetOutput())
verts.SetSource(sphere.GetOutput())
sphereMapper = vtkPolyDataMapper()
sphereMapper.SetInput(verts.GetOutput())
vertActor = vtkActor()
vertActor.SetMapper(sphereMapper)
vertActor.GetProperty().SetColor(0,0,1)

# assign our actor to the renderer
ren.AddActor(cubeActor)
ren.AddActor(vertActor)

# enable user interface interactor
iren.Initialize()

#renWin SetFileName "cubeEdges.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
#wm withdraw .

iren.Start()
