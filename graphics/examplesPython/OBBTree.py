#!/usr/local/bin/python

from libVTKCommonPython import *
from libVTKGraphicsPython import *

#catch  load vtktcl 
# get the interactor ui
#source ../../examplesTcl/vtkInt.tcl

reader = vtkSTLReader()
reader.SetFileName("../../../vtkdata/42400-IDGH.stl")
dataMapper = vtkPolyDataMapper()
dataMapper.SetInput(reader.GetOutput())
model = vtkActor()
model.SetMapper(dataMapper)
model.GetProperty().SetColor(1,0,0)

obb = vtkOBBTree()
obb.SetMaxLevel(4)
obb.SetNumberOfCellsPerBucket(4)
boxes = vtkSpatialRepresentationFilter()
boxes.SetInput(reader.GetOutput())
boxes.SetSpatialRepresentation(obb)
boxMapper = vtkPolyDataMapper()
boxMapper.SetInput(boxes.GetOutput())
boxActor = vtkActor()
boxActor.SetMapper(boxMapper)
boxActor.GetProperty().SetAmbient(1)
boxActor.GetProperty().SetDiffuse(0)
boxActor.GetProperty().SetRepresentationToWireframe()

ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(model)
ren.AddActor(boxActor)
ren.SetBackground(0.1,0.2,0.4)
renWin.SetSize(500,500)
ren.GetActiveCamera().Zoom(1.5)

# render the image
#
iren.Initialize()
#renWin SetFileName OBBTree.tcl.ppm
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
#wm withdraw .

iren.Start()
