#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

#
# Demonstrate the use of clipping and capping on polyhedral data
#
from colors import *
# create a sphere and clip it
#
sphere = vtkSphereSource()
sphere.SetRadius(1)
sphere.SetPhiResolution(10)
sphere.SetThetaResolution(10)
plane = vtkPlane()
plane.SetOrigin(0,0,0)
plane.SetNormal(-1,-1,0)
clipper = vtkClipPolyData()
clipper.SetInput(sphere.GetOutput())
clipper.SetClipFunction(plane)
clipper.GenerateClipScalarsOn()
clipper.GenerateClippedOutputOn()
clipper.SetValue(0)
clipMapper = vtkPolyDataMapper()
clipMapper.SetInput(clipper.GetOutput())
clipMapper.ScalarVisibilityOff()

backProp = vtkProperty()
backProp.SetDiffuseColor(tomato)
clipActor = vtkActor()
clipActor.SetMapper(clipMapper)
clipActor.GetProperty().SetColor(peacock[0],peacock[1],peacock[2])
clipActor.SetBackfaceProperty(backProp)

# now extract feature edges
boundaryEdges = vtkFeatureEdges()
boundaryEdges.SetInput(clipper.GetOutput())
boundaryEdges.BoundaryEdgesOn()
boundaryEdges.FeatureEdgesOff()
boundaryEdges.NonManifoldEdgesOff()

boundaryClean = vtkCleanPolyData()
boundaryClean.SetInput(boundaryEdges.GetOutput())

boundaryStrips = vtkStripper()
boundaryStrips.SetInput(boundaryClean.GetOutput())
boundaryStrips.Update()

boundaryPoly = vtkPolyData()
boundaryPoly.SetPoints(boundaryStrips.GetOutput().GetPoints())
boundaryPoly.SetPolys(boundaryStrips.GetOutput().GetLines())

boundaryTriangles = vtkTriangleFilter()
boundaryTriangles.SetInput(boundaryPoly)


boundaryMapper = vtkPolyDataMapper()
boundaryMapper.SetInput(boundaryPoly)

boundaryActor = vtkActor()
boundaryActor.SetMapper(boundaryMapper)
boundaryActor.GetProperty().SetColor(banana[0],banana[1],banana[2])

# Create graphics stuff
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(clipActor)
ren.AddActor(boundaryActor)
ren.SetBackground(1,1,1)
ren.GetActiveCamera().Azimuth(30)
ren.GetActiveCamera().Elevation(30)
ren.GetActiveCamera().Dolly(1.2)
ren.ResetCameraClippingRange()

renWin.SetSize(400,400)
iren.Initialize()

# render the image
#

iren.Start()
