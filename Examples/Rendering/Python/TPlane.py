#!/usr/bin/env python

# This simple example shows how to do basic texture mapping.

import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Load in the texture map. A texture is any unsigned char image. If it
# is not of this type, you will have to map it through a lookup table
# or by using vtkImageShiftScale.
bmpReader = vtk.vtkBMPReader()
bmpReader.SetFileName(VTK_DATA_ROOT + "/Data/masonry.bmp")
atext = vtk.vtkTexture()
atext.SetInputConnection(bmpReader.GetOutputPort())
atext.InterpolateOn()

# Create a plane source and actor. The vtkPlanesSource generates
# texture coordinates.
plane = vtk.vtkPlaneSource()
planeMapper = vtk.vtkPolyDataMapper()
planeMapper.SetInputConnection(plane.GetOutputPort())
planeActor = vtk.vtkActor()
planeActor.SetMapper(planeMapper)
planeActor.SetTexture(atext)

# Create the RenderWindow, Renderer and both Actors
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
ren.AddActor(planeActor)
ren.SetBackground(0.1, 0.2, 0.4)
renWin.SetSize(500, 500)

ren.ResetCamera()
cam1 = ren.GetActiveCamera()
cam1.Elevation(-30)
cam1.Roll(-20)
ren.ResetCameraClippingRange()

iren.Initialize()
renWin.Render()
iren.Start()
