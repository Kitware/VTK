#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# pipeline stuff
#
sphere = vtk.vtkSphereSource()
sphere.SetPhiResolution(10)
sphere.SetThetaResolution(20)

xform = vtk.vtkTransformCoordinateSystems()
xform.SetInputConnection(sphere.GetOutputPort())
xform.SetInputCoordinateSystemToWorld()
xform.SetOutputCoordinateSystemToDisplay()
xform.SetViewport(ren1)

gs = vtk.vtkGlyphSource2D()
gs.SetGlyphTypeToCircle()
gs.SetScale(20)
gs.FilledOff()
gs.CrossOn()
gs.Update()

# Create a table of glyphs
glypher = vtk.vtkGlyph2D()
glypher.SetInputConnection(xform.GetOutputPort())
glypher.SetSourceData(0, gs.GetOutput())
glypher.SetScaleModeToDataScalingOff()

mapper = vtk.vtkPolyDataMapper2D()
mapper.SetInputConnection(glypher.GetOutputPort())

glyphActor = vtk.vtkActor2D()
glyphActor.SetMapper(mapper)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(glyphActor)
ren1.SetBackground(0, 0, 0)

renWin.SetSize(300, 300)

iren.Initialize()
renWin.Render()

# render the image
#iren.Start()
