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

# read data
#
input = vtkPolyDataReader()
input.SetFileName(VTK_DATA + "/brainImageSmooth.vtk")

#
# generate vectors
clean = vtkCleanPolyData()
clean.SetInput(input.GetOutput())

smooth = vtkWindowedSincPolyDataFilter()
smooth.SetInput(clean.GetOutput())
smooth.GenerateErrorVectorsOn()
smooth.GenerateErrorScalarsOn()
smooth.Update()

mapper = vtkPolyDataMapper()
mapper.SetInput(smooth.GetOutput())
mapper.SetScalarRange(smooth.GetOutput().GetScalarRange())

brain = vtkActor()
brain.SetMapper(mapper)


# Add the actors to the renderer, set the background and size
#
ren.AddActor(brain)

renWin.SetSize(320,240)

#cam1=ren.GetActiveCamera()
#cam1.SetPosition(152.589,-135.901,173.068)
#cam1.SetFocalPoint(146.003,22.3839,0.260541)
#cam1.SetViewUp(-0.255578,-0.717754,-0.647695)

iren.Initialize()
renWin.Render()

#
# test the writers
byu = vtkBYUWriter()
byu.SetGeometryFileName("brain.g")
byu.SetScalarFileName("brain.s")
byu.SetDisplacementFileName("brain.d")
byu.SetTextureFileName("brain.t")
byu.SetInput(smooth.GetOutput())
byu.Write()

dsw = vtkDataSetWriter()
dsw.SetInput(smooth.GetOutput())
dsw.SetFileName("brain.dsw")
dsw.Write()

pdw = vtkPolyDataWriter()
pdw.SetInput(smooth.GetOutput())
pdw.SetFileName("brain.pdw")
pdw.Write()

#
# the next writers only handle triangles
triangles = vtkTriangleFilter()
triangles.SetInput(smooth.GetOutput())

mcubes = vtkMCubesWriter()
mcubes.SetInput(triangles.GetOutput())
mcubes.SetFileName("brain.tri")
mcubes.Write()

stl = vtkSTLWriter()
stl.SetInput(triangles.GetOutput())
stl.SetFileName("brain.stl")
stl.Write()

iren.Start()
