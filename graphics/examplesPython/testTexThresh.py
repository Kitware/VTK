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

# construct simple pixmap with test scalars
#
plane = vtkStructuredPoints()
plane.SetDimensions(3,3,1)
scalars = vtkScalars()
scalars.InsertScalar(0,0.0)
scalars.InsertScalar(1,1.0)
scalars.InsertScalar(2,0.0)
scalars.InsertScalar(3,1.0)
scalars.InsertScalar(4,2.0)
scalars.InsertScalar(5,1.0)
scalars.InsertScalar(6,0.0)
scalars.InsertScalar(7,1.0)
scalars.InsertScalar(8,0.0)
plane.GetPointData().SetScalars(scalars)

# read in texture map
#
tmap = vtkStructuredPointsReader()
tmap.SetFileName(VTK_DATA + "/texThres2.vtk")
texture = vtkTexture()
texture.SetInput(tmap.GetOutput())
texture.InterpolateOff()
texture.RepeatOff()

# Cut data with texture
#
planePolys = vtkStructuredPointsGeometryFilter()
planePolys.SetInput(plane)
planePolys.SetExtent(0,3,0,3,0,0)
thresh = vtkThresholdTextureCoords()
#    thresh SetInput plane
thresh.SetInput(planePolys.GetOutput())
thresh.ThresholdByUpper(0.5)
planeMap = vtkDataSetMapper()
planeMap.SetInput(thresh.GetOutput())
planeMap.SetScalarRange(0,2)
planeActor = vtkActor()
planeActor.SetMapper(planeMap)
planeActor.SetTexture(texture)

# we set the opacity to 0.999 to indicate that we are doing stuff with
# alpha. Ideally we shouldn't have to do this, but leaving the alpha
# funcs on all the time in OpenGL kills performance on some systems.
planeActor.GetProperty().SetOpacity(0.999)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(planeActor)
ren.SetBackground(0.5,0.5,0.5)
renWin.SetSize(450,450)

iren.Initialize()

# render the image
#









iren.Start()
