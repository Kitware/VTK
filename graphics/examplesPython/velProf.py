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

# create pipeline
#
pl3d = vtkPLOT3DReader()
pl3d.SetXYZFileName("../../../vtkdata/combxyz.bin")
pl3d.SetQFileName("../../../vtkdata/combq.bin")
pl3d.SetScalarFunctionNumber(100)
pl3d.SetVectorFunctionNumber(202)
pl3d.Update()
plane = vtkStructuredGridGeometryFilter()
plane.SetInput(pl3d.GetOutput())
plane.SetExtent(10,10,1,100,1,100)
plane2 = vtkStructuredGridGeometryFilter()
plane2.SetInput(pl3d.GetOutput())
plane2.SetExtent(30,30,1,100,1,100)
plane3 = vtkStructuredGridGeometryFilter()
plane3.SetInput(pl3d.GetOutput())
plane3.SetExtent(45,45,1,100,1,100)
appendF = vtkAppendPolyData()
appendF.AddInput(plane.GetOutput())
appendF.AddInput(plane2.GetOutput())
appendF.AddInput(plane3.GetOutput())
warp = vtkWarpVector()
warp.SetInput(appendF.GetOutput())
warp.SetScaleFactor(0.005)
normals = vtkPolyDataNormals()
normals.SetInput(warp.GetPolyDataOutput())
normals.SetFeatureAngle(60)
planeMapper = vtkDataSetMapper()
planeMapper.SetInput(normals.GetOutput())
planeMapper.SetScalarRange(0.197813,0.710419)
planeMapper.ScalarVisibilityOff()
planeActor = vtkActor()
planeActor.SetMapper(planeMapper)
planeProp=planeActor.GetProperty()
planeProp.SetColor(salmon[0],salmon[1],salmon[2])

outline = vtkStructuredGridOutlineFilter()
outline.SetInput(pl3d.GetOutput())
outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInput(outline.GetOutput())
outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineProp=outlineActor.GetProperty()
outlineProp.SetColor(black[0],black[1],black[2])

ren.AddActor(outlineActor)
ren.AddActor(planeActor)
ren.SetBackground(1,1,1)
renWin.SetSize(500,500)
iren.Initialize()
ren.GetActiveCamera().Zoom(1.4)

# render the image
#
renWin.Render()
#renWin SetFileName "velProf.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
#wm withdraw .



iren.Start()
