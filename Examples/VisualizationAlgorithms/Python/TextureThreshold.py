#!/usr/bin/env python

# This example shows how to use a transparent texture map to perform
# thresholding. The key is the vtkThresholdTextureCoords filter which
# creates texture coordinates based on a threshold value. These
# texture coordinates are used in conjuntion with a texture map with
# varying opacity and intensity to create an inside, transition, and
# outside region.

import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Begin by reading some structure grid data.
pl3d = vtk.vtkPLOT3DReader()
pl3d.SetXYZFileName(VTK_DATA_ROOT + "/Data/bluntfinxyz.bin")
pl3d.SetQFileName(VTK_DATA_ROOT + "/Data/bluntfinq.bin")
pl3d.SetScalarFunctionNumber(100)
pl3d.SetVectorFunctionNumber(202)
pl3d.Update()

# Now extract surfaces from the grid corresponding to boundary
# geometry.  First the wall.
wall = vtk.vtkStructuredGridGeometryFilter()
wall.SetInput(pl3d.GetOutput())
wall.SetExtent(0, 100, 0, 0, 0, 100)
wallMap = vtk.vtkPolyDataMapper()
wallMap.SetInput(wall.GetOutput())
wallMap.ScalarVisibilityOff()
wallActor = vtk.vtkActor()
wallActor.SetMapper(wallMap)
wallActor.GetProperty().SetColor(0.8, 0.8, 0.8)

# Now the fin.
fin = vtk.vtkStructuredGridGeometryFilter()
fin.SetInput(pl3d.GetOutput())
fin.SetExtent(0, 100, 0, 100, 0, 0)
finMap = vtk.vtkPolyDataMapper()
finMap.SetInput(fin.GetOutput())
finMap.ScalarVisibilityOff()
finActor = vtk.vtkActor()
finActor.SetMapper(finMap)
finActor.GetProperty().SetColor(0.8, 0.8, 0.8)

# Extract planes to threshold. Start by reading the specially designed
# texture map that has three regions: an inside, boundary, and outside
# region. The opacity and intensity of this texture map are varied.
tmap = vtk.vtkStructuredPointsReader()
tmap.SetFileName(VTK_DATA_ROOT + "/Data/texThres2.vtk")
texture = vtk.vtkTexture()
texture.SetInput(tmap.GetOutput())
texture.InterpolateOff()
texture.RepeatOff()

# Here are the three planes which will be texture thresholded.
plane1 = vtk.vtkStructuredGridGeometryFilter()
plane1.SetInput(pl3d.GetOutput())
plane1.SetExtent(10, 10, 0, 100, 0, 100)
thresh1 = vtk.vtkThresholdTextureCoords()
thresh1.SetInput(plane1.GetOutput())
thresh1.ThresholdByUpper(1.5)
plane1Map = vtk.vtkDataSetMapper()
plane1Map.SetInput(thresh1.GetOutput())
plane1Map.SetScalarRange(pl3d.GetOutput().GetScalarRange())
plane1Actor = vtk.vtkActor()
plane1Actor.SetMapper(plane1Map)
plane1Actor.SetTexture(texture)
plane1Actor.GetProperty().SetOpacity(0.999)

plane2 = vtk.vtkStructuredGridGeometryFilter()
plane2.SetInput(pl3d.GetOutput())
plane2.SetExtent(30, 30, 0, 100, 0, 100)
thresh2 = vtk.vtkThresholdTextureCoords()
thresh2.SetInput(plane2.GetOutput())
thresh2.ThresholdByUpper(1.5)
plane2Map = vtk.vtkDataSetMapper()
plane2Map.SetInput(thresh2.GetOutput())
plane2Map.SetScalarRange(pl3d.GetOutput().GetScalarRange())
plane2Actor = vtk.vtkActor()
plane2Actor.SetMapper(plane2Map)
plane2Actor.SetTexture(texture)
plane2Actor.GetProperty().SetOpacity(0.999)

plane3 = vtk.vtkStructuredGridGeometryFilter()
plane3.SetInput(pl3d.GetOutput())
plane3.SetExtent(35, 35, 0, 100, 0, 100)
thresh3 = vtk.vtkThresholdTextureCoords()
thresh3.SetInput(plane3.GetOutput())
thresh3.ThresholdByUpper(1.5)
plane3Map = vtk.vtkDataSetMapper()
plane3Map.SetInput(thresh3.GetOutput())
plane3Map.SetScalarRange(pl3d.GetOutput().GetScalarRange())
plane3Actor = vtk.vtkActor()
plane3Actor.SetMapper(plane3Map)
plane3Actor.SetTexture(texture)
plane3Actor.GetProperty().SetOpacity(0.999)

# For context create an outline around the data.
outline = vtk.vtkStructuredGridOutlineFilter()
outline.SetInput(pl3d.GetOutput())
outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInput(outline.GetOutput())
outlineActor = vtk.vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineProp = outlineActor.GetProperty()
outlineProp.SetColor(0, 0, 0)

# Create the RenderWindow, Renderer and both Actors
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
ren.AddActor(outlineActor)
ren.AddActor(wallActor)
ren.AddActor(finActor)
ren.AddActor(plane1Actor)
ren.AddActor(plane2Actor)
ren.AddActor(plane3Actor)
ren.SetBackground(1, 1, 1)
renWin.SetSize(500, 500)

# Set up a nice view.
cam1 = vtk.vtkCamera()
cam1.SetClippingRange(1.51176, 75.5879)
cam1.SetFocalPoint(2.33749, 2.96739, 3.61023)
cam1.SetPosition(10.8787, 5.27346, 15.8687)
cam1.SetViewAngle(30)
cam1.SetViewUp(-0.0610856, 0.987798, -0.143262)
ren.SetActiveCamera(cam1)

iren.Initialize()
renWin.Render()
iren.Start()
