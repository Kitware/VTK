#!/usr/local/bin/python

from libVTKCommonPython import *
from libVTKGraphicsPython import *

#catch  load vtktcl 
# create selected cones
# get the interactor ui
#source ../../examplesTcl/vtkInt.tcl
#source ../../examplesTcl/colors.tcl
from colors import *
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create pipeline
#
reader = vtkStructuredPointsReader()
reader.SetFileName("../../../vtkdata/carotid.vtk")
threshold = vtkThresholdPoints()
threshold.SetInput(reader.GetOutput())
threshold.ThresholdByUpper(200)
mask = vtkMaskPoints()
mask.SetInput(threshold.GetOutput())
mask.SetOnRatio(10)
cone = vtkConeSource()
cone.SetResolution(3)
cone.SetHeight(1)
cone.SetRadius(0.25)
cones = vtkGlyph3D()
cones.SetInput(mask.GetOutput())
cones.SetSource(cone.GetOutput())
cones.SetScaleFactor(0.5)
cones.SetScaleModeToScaleByVector()
lut = vtkLookupTable()
lut.SetHueRange(.667,0.0)
lut.Build()
vecMapper = vtkPolyDataMapper()
vecMapper.SetInput(cones.GetOutput())
vecMapper.SetScalarRange(2,10)
vecMapper.SetLookupTable(lut)
vecActor = vtkActor()
vecActor.SetMapper(vecMapper)

# contours of speed
iso = vtkContourFilter()
iso.SetInput(reader.GetOutput())
iso.SetValue(0,190)
isoMapper = vtkPolyDataMapper()
isoMapper.SetInput(iso.GetOutput())
isoMapper.ScalarVisibilityOff()
isoActor = vtkActor()
isoActor.SetMapper(isoMapper)
isoActor.GetProperty().SetRepresentationToWireframe()
isoActor.GetProperty().SetOpacity(0.25)

# outline
outline = vtkOutlineFilter()
outline.SetInput(reader.GetOutput())
outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInput(outline.GetOutput())
outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineProp=outlineActor.GetProperty()
outlineProp.SetColor(0,0,0)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(outlineActor)
ren.AddActor(vecActor)
ren.AddActor(isoActor)
ren.SetBackground(1,1,1)
renWin.SetSize(500,500)
#renWin SetSize 1000 1000
ren.SetBackground(0.1,0.2,0.4)
ren.GetActiveCamera().Zoom(1.5)
iren.Initialize()

# render the image
#

#renWin SetFileName "thrshldV.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
#wm withdraw .
iren.Start()
