#!/usr/local/bin/python

from libVTKCommonPython import *
from libVTKGraphicsPython import *

#catch  load vtktcl 
## LOx post CFD case study

# get helper scripts
#source ../../examplesTcl/vtkInt.tcl
#source ../../examplesTcl/colors.tcl
from colors import *
# read data
#
pl3d = vtkPLOT3DReader()
pl3d.SetXYZFileName("../../../vtkdata/postxyz.bin")
pl3d.SetQFileName("../../../vtkdata/postq.bin")
pl3d.Update()

# computational planes
floorComp = vtkStructuredGridGeometryFilter()
floorComp.SetExtent(0,37,0,75,0,0)
floorComp.SetInput(pl3d.GetOutput())
floorComp.Update()
floorMapper = vtkPolyDataMapper()
floorMapper.SetInput(floorComp.GetOutput())
floorMapper.ScalarVisibilityOff()
floorActor = vtkActor()
floorActor.SetMapper(floorMapper)
floorActor.GetProperty().SetRepresentationToWireframe()
floorActor.GetProperty().SetColor(0,0,0)

postComp = vtkStructuredGridGeometryFilter()
postComp.SetExtent(10,10,0,75,0,37)
postComp.SetInput(pl3d.GetOutput())
postMapper = vtkPolyDataMapper()
postMapper.SetInput(postComp.GetOutput())
postMapper.ScalarVisibilityOff()
postActor = vtkActor()
postActor.SetMapper(postMapper)
postActor.GetProperty().SetColor(0,0,0)
postActor.GetProperty().SetRepresentationToWireframe()

fanComp = vtkStructuredGridGeometryFilter()
fanComp.SetExtent(0,37,38,38,0,37)
fanComp.SetInput(pl3d.GetOutput())
fanMapper = vtkPolyDataMapper()
fanMapper.SetInput(fanComp.GetOutput())
fanActor = vtkActor()
fanActor.SetMapper(fanMapper)
fanActor.GetProperty().SetColor(0,0,0)
fanActor.GetProperty().SetRepresentationToWireframe()

# outline
outline = vtkStructuredGridOutlineFilter()
outline.SetInput(pl3d.GetOutput())
outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInput(outline.GetOutput())
outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineProp=outlineActor.GetProperty()
outlineProp.SetColor(0,0,0)

# Create graphics stuff
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(outlineActor)
ren.AddActor(floorActor)
ren.AddActor(postActor)
ren.AddActor(fanActor)

aCam = vtkCamera()
aCam.SetFocalPoint(0.00657892,0,2.41026)
aCam.SetPosition(-1.94838,-47.1275,39.4607)
aCam.ComputeViewPlaneNormal()
aCam.SetViewPlaneNormal(-0.0325936,-0.785725,0.617717)
aCam.SetViewUp(0.00653193,0.617865,0.786257)

ren.SetBackground(1,1,1)
ren.SetActiveCamera(aCam)
renWin.SetSize(400,400)

iren.Initialize()
renWin.Render()

# render the image
#

renWin.Render()
#renWin SetFileName "LOxGrid.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
#wm withdraw .






iren.Start()
