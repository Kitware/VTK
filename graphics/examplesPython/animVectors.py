#!/usr/local/bin/python

from libVTKCommonPython import *
from libVTKGraphicsPython import *

#catch  load vtktcl 
# get the interactor ui
#source ../../examplesTcl/vtkInt.tcl
#source ../../examplesTcl/colors.tcl
from colors import *
# Create the RenderWindow, Renderer and both Actors
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# read data
#
reader = vtkStructuredPointsReader()
reader.SetFileName("../../../vtkdata/carotid.vtk")
threshold = vtkThresholdPoints()
threshold.SetInput(reader.GetOutput())
threshold.ThresholdByUpper(200)

line = vtkLineSource()
line.SetResolution(1)
lines = vtkGlyph3D()
lines.SetInput(threshold.GetOutput())
lines.SetSource(line.GetOutput())
lines.SetScaleFactor(0.005)
lines.SetScaleModeToScaleByScalar()
lines.Update() #make.range.current()
vectorMapper = vtkPolyDataMapper()
vectorMapper.SetInput(lines.GetOutput())
vectorMapper.SetScalarRange(lines.GetOutput().GetScalarRange())
vectorMapper.ImmediateModeRenderingOn()
vectorActor = vtkActor()
vectorActor.SetMapper(vectorMapper)
vectorActor.GetProperty().SetOpacity(0.99)
# 8 texture maps
tmap1 = vtkStructuredPointsReader()
tmap1.SetFileName("../../../vtkdata/vecTex/vecAnim1.vtk")
texture1 = vtkTexture()
texture1.SetInput(tmap1.GetOutput())
texture1.InterpolateOff()
texture1.RepeatOff()

tmap2 = vtkStructuredPointsReader()
tmap2.SetFileName("../../../vtkdata/vecTex/vecAnim2.vtk")
texture2 = vtkTexture()
texture2.SetInput(tmap2.GetOutput())
texture2.InterpolateOff()
texture2.RepeatOff()

tmap3 = vtkStructuredPointsReader()
tmap3.SetFileName("../../../vtkdata/vecTex/vecAnim3.vtk")
texture3 = vtkTexture()
texture3.SetInput(tmap3.GetOutput())
texture3.InterpolateOff()
texture3.RepeatOff()

tmap4 = vtkStructuredPointsReader()
tmap4.SetFileName("../../../vtkdata/vecTex/vecAnim4.vtk")
texture4 = vtkTexture()
texture4.SetInput(tmap4.GetOutput())
texture4.InterpolateOff()
texture4.RepeatOff()

tmap5 = vtkStructuredPointsReader()
tmap5.SetFileName("../../../vtkdata/vecTex/vecAnim5.vtk")
texture5 = vtkTexture()
texture5.SetInput(tmap5.GetOutput())
texture5.InterpolateOff()
texture5.RepeatOff()

tmap6 = vtkStructuredPointsReader()
tmap6.SetFileName("../../../vtkdata/vecTex/vecAnim6.vtk")
texture6 = vtkTexture()
texture6.SetInput(tmap6.GetOutput())
texture6.InterpolateOff()
texture6.RepeatOff()

tmap7 = vtkStructuredPointsReader()
tmap7.SetFileName("../../../vtkdata/vecTex/vecAnim7.vtk")
texture7 = vtkTexture()
texture7.SetInput(tmap7.GetOutput())
texture7.InterpolateOff()
texture7.RepeatOff()

tmap8 = vtkStructuredPointsReader()
tmap8.SetFileName("../../../vtkdata/vecTex/vecAnim8.vtk")
texture8 = vtkTexture()
texture8.SetInput(tmap8.GetOutput())
texture8.InterpolateOff()
texture8.RepeatOff()

vectorActor.SetTexture(texture1)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(vectorActor)
ren.SetBackground(1,1,1)
renWin.SetSize(500,500)

iren.Initialize()

# render the image
#
ren.GetActiveCamera().Zoom(1.5)
renWin.Render()

# prevent the tk window from showing up then start the event loop
#wm withdraw .

# go into loop
for i in range(0,5):
	vectorActor.SetTexture(texture1)
	renWin.Render()
	vectorActor.SetTexture(texture2)
	renWin.Render()
	vectorActor.SetTexture(texture3)
	renWin.Render()
	vectorActor.SetTexture(texture4)
	renWin.Render()
	vectorActor.SetTexture(texture5)
	renWin.Render()
	vectorActor.SetTexture(texture6)
	renWin.Render()
	vectorActor.SetTexture(texture7)
	renWin.Render()
	vectorActor.SetTexture(texture8)
	renWin.Render()
	vectorActor.SetTexture(texture1)
	renWin.Render()
	vectorActor.SetTexture(texture2)
	renWin.Render()
	vectorActor.SetTexture(texture3)
	renWin.Render()
	vectorActor.SetTexture(texture4)
	renWin.Render()
	vectorActor.SetTexture(texture5)
	renWin.Render()
	vectorActor.SetTexture(texture6)
	renWin.Render()
	vectorActor.SetTexture(texture7)
	renWin.Render()
	vectorActor.SetTexture(texture8)
	renWin.Render()
  
#renWin SetFileName animVectors.tcl.ppm
#renWin SaveImageAsPPM
iren.Start()
