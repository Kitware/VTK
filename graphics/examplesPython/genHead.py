#!/usr/local/bin/python

from libVTKCommonPython import *
from libVTKGraphicsPython import *
from libVTKPatentedPython import *

#catch  load vtktcl 
# Generate marching cubes head model (full resolution)

# get the interactor ui and colors
#source ../../examplesTcl/vtkInt.tcl
#source ../../examplesTcl/colors.tcl
from colors import *
# create pipeline
# reader reads slices
v16 = vtkVolume16Reader()
v16.SetDataDimensions(256,256)
v16.SetDataByteOrderToLittleEndian()
v16.SetFilePrefix("../../../vtkdata/fullHead/headsq")
v16.SetDataSpacing(0.8,0.8,1.5)
v16.SetImageRange(30,50)
v16.SetDataMask(0x7fff)

# write isosurface to file
mcubes = vtkSliceCubes()
mcubes.SetReader(v16)
mcubes.SetValue(1150)
mcubes.SetFileName("fullHead.tri")
mcubes.SetLimitsFileName("fullHead.lim")
mcubes.Update()

# read from file
reader = vtkMCubesReader()
reader.SetFileName("fullHead.tri")
reader.SetLimitsFileName("fullHead.lim")

mapper = vtkPolyDataMapper()
mapper.SetInput(reader.GetOutput())
    
head = vtkActor()
head.SetMapper(mapper)
head.GetProperty().SetColor(raw_sienna[0],raw_sienna[1],raw_sienna[2])

# Create the RenderWindow, Renderer and Interactor
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(head)
ren.SetBackground(1,1,1)
renWin.SetSize(500,500)
ren.SetBackground(slate_grey)
ren.GetActiveCamera().Zoom(1.5)
ren.GetActiveCamera().Elevation(90)

# render the image
#

iren.Initialize()

#renWin SetFileName "genHead.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
#wm withdraw .

iren.Start()
