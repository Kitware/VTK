#!/usr/bin/env python

# Generate texture coordinates on a "random" sphere.

import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *
from libVTKImagingPython import *

# create some random points in a sphere
#
sphere = vtkPointSource()
sphere.SetNumberOfPoints(25)

# triangulate the points
#
delaunay = vtkDelaunay3D()
delaunay.SetInput(sphere.GetOutput())
delaunay.SetTolerance(0.01)
    
# texture map the sphere (using cylindrical coordinate system)
#
tmapper = vtkTextureMapToCylinder()
tmapper.SetInput(delaunay.GetOutput())
tmapper.PreventSeamOn()

xform = vtkTransformTextureCoords()
xform.SetInput(tmapper.GetOutput())
xform.SetScale((4,4,1))

mapper = vtkDataSetMapper()
mapper.SetInput(xform.GetOutput())

# load in the texture map and assign to actor
#
pnmReader = vtkPNMReader()
pnmReader.SetFileName(VTK_DATA + "/masonry.ppm")
atext = vtkTexture()
atext.SetInput(pnmReader.GetOutput())
atext.InterpolateOn()
triangulation = vtkActor()
triangulation.SetMapper(mapper)
triangulation.SetTexture(atext)

# Create rendering stuff
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(triangulation)
ren1.SetBackground(1,1,1)
renWin.SetSize(500,500)
renWin.Render()

# render the image
#
renWin.Render()
renWin.SetFileName("valid/cylMap.ppm")

iren.Start()
