#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create the RenderWindow, Renderer and both Actors
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# load in the texture map
#
pngReader = vtk.vtkPNGReader()
pngReader.SetFileName(VTK_DATA_ROOT + "/Data/camscene.png")
pngReader.Update()

xWidth = pngReader.GetOutput().GetDimensions()[0]
yHeight = pngReader.GetOutput().GetDimensions()[1]

wl = vtk.vtkWarpLens()
wl.SetInputConnection(pngReader.GetOutputPort())
wl.SetPrincipalPoint(2.4507, 1.7733)
wl.SetFormatWidth(4.792)
wl.SetFormatHeight(3.6)
wl.SetImageWidth(xWidth)
wl.SetImageHeight(yHeight)
wl.SetK1(0.01307)
wl.SetK2(0.0003102)
wl.SetP1(1.953e-005)
wl.SetP2(-9.655e-005)

gf = vtk.vtkGeometryFilter()
gf.SetInputConnection(wl.GetOutputPort())

tf = vtk.vtkTriangleFilter()
tf.SetInputConnection(gf.GetOutputPort())

strip = vtk.vtkStripper()
strip.SetInputConnection(tf.GetOutputPort())
strip.SetMaximumLength(250)

dsm = vtk.vtkPolyDataMapper()
dsm.SetInputConnection(strip.GetOutputPort())

planeActor = vtk.vtkActor()
planeActor.SetMapper(dsm)

# Add the actors to the renderer, set the background and size
ren1.AddActor(planeActor)
ren1.SetBackground(0.1, 0.2, 0.4)

renWin.SetSize(300, 300)

# render the image
iren.Initialize()

renWin.Render()

ren1.GetActiveCamera().Zoom(1.4)

renWin.Render()

#iren.Start()
