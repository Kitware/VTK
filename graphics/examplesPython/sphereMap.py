#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *
from libVTKImagingPython import *

# Generate texture coordinates on a "random" sphere.

# create some random points in a sphere
#
sphere = vtkPointSource()
sphere.SetNumberOfPoints(25)

# triangulate the points
#
delny = vtkDelaunay3D()
delny.SetInput(sphere.GetOutput())
delny.SetTolerance(0.01)
    
# texture map the sphere
#
tmapper = vtkTextureMapToSphere()
tmapper.SetInput(delny.GetOutput())
tmapper.PreventSeamOn()

xform = vtkTransformTextureCoords()
xform.SetInput(tmapper.GetOutput())
xform.SetScale(4,4,1)

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
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(triangulation)
ren.SetBackground(1,1,1)
renWin.SetSize(500,500)

# render the image
#
renWin.Render()
renWin.SetFileName("valid/sphereMap.ppm")

iren.Start()
