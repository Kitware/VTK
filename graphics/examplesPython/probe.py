#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

from colors import *
# Create the RenderWindow, Renderer and both Actors
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# cut data
pl3d = vtkPLOT3DReader()
pl3d.SetXYZFileName(VTK_DATA + "/combxyz.bin")
pl3d.SetQFileName(VTK_DATA + "/combq.bin")
pl3d.SetScalarFunctionNumber(100)
pl3d.SetVectorFunctionNumber(202)
pl3d.Update()
plane = vtkPlane()
plane.SetOrigin(pl3d.GetOutput().GetCenter())
plane.SetNormal(-0.287,0,0.9579)
planeCut = vtkCutter()
planeCut.SetInput(pl3d.GetOutput())
planeCut.SetCutFunction(plane)
probe = vtkProbeFilter()
probe.SetInput(planeCut.GetOutput())
probe.SetSource(pl3d.GetOutput())
cutMapper = vtkDataSetMapper()
cutMapper.SetInput(probe.GetOutput())
cutMapper.SetScalarRange(  \
	pl3d.GetOutput().GetPointData().GetScalars().GetRange() )
cutActor = vtkActor()
cutActor.SetMapper(cutMapper)

#extract plane
compPlane = vtkStructuredGridGeometryFilter()
compPlane.SetInput(pl3d.GetOutput())
compPlane.SetExtent(0,100,0,100,9,9)
planeMapper = vtkPolyDataMapper()
planeMapper.SetInput(compPlane.GetOutput())
planeMapper.ScalarVisibilityOff()
planeActor = vtkActor()
planeActor.SetMapper(planeMapper)
planeActor.GetProperty().SetRepresentationToWireframe()
planeActor.GetProperty().SetColor(0,0,0)

#outline
outline = vtkStructuredGridOutlineFilter()
outline.SetInput(pl3d.GetOutput())
outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInput(outline.GetOutput())
outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineProp=outlineActor.GetProperty()
outlineProp.SetColor(0,0,0)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(outlineActor)
ren.AddActor(planeActor)
ren.AddActor(cutActor)
ren.SetBackground(1,1,1)
renWin.SetSize(500,500)

cam1=ren.GetActiveCamera()
cam1.SetClippingRange(3.95297,50)
cam1.SetFocalPoint(9.71821,0.458166,29.3999)
cam1.SetPosition(2.7439,-37.3196,38.7167)
cam1.SetViewUp(-0.16123,0.264271,0.950876)
iren.Initialize()

# render the image
#





iren.Start()
