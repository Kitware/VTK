#!/usr/local/bin/python

from libVTKCommonPython import *
from libVTKGraphicsPython import *
from libVTKImagingPython import *

#catch  load vtktcl 
# Generate texture coordinates on a "random" sphere.

# get the interactor ui
#source ../../examplesTcl/vtkInt.tcl

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
pnmReader.SetFileName("../../../vtkdata/masonry.ppm")
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
renWin.SetFileName("valid/sphereMap.tcl.ppm")
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
#wm withdraw .



iren.Start()
