#!/usr/local/bin/python

from libVTKCommonPython import *
from libVTKGraphicsPython import *
from libVTKImagingPython import *

# Mix imaging and visualization; warp an image in z-direction
#

#catch  load vtktcl 
#source ../../examplesTcl/vtkInt.tcl

# read in some structured points
#
reader = vtkPNMReader()
reader.SetFileName("../../../vtkdata/masonry.ppm")

luminance = vtkImageLuminance()
luminance.SetInput(reader.GetOutput())

geometry = vtkStructuredPointsGeometryFilter()
geometry.SetInput(luminance.GetOutput())

warp = vtkWarpScalar()
warp.SetInput(geometry.GetOutput())
warp.SetScaleFactor(-0.1)

#
# use merge to put back scalars from image file
#
merge = vtkMergeFilter()
merge.SetGeometry(warp.GetOutput())
merge.SetScalars(reader.GetOutput())

mapper = vtkDataSetMapper()
mapper.SetInput(merge.GetOutput())
mapper.SetScalarRange(0,255)
mapper.ImmediateModeRenderingOff()

actor = vtkActor()
actor.SetMapper(mapper)

# Create renderer stuff
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(actor)
ren.GetActiveCamera().Azimuth(20)
ren.GetActiveCamera().Elevation(30)
ren.SetBackground(0.1,0.2,0.4)
renWin.SetSize(450,450)

# render the image
#
cam1=ren.GetActiveCamera()
cam1.Zoom(1.4)
iren.Initialize()

#renWin SetFileName "valid/imageWarp.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
#wm withdraw .


iren.Start()
