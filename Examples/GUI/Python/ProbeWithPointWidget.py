#!/usr/bin/env python

# This example demonstrates how to use the vtkPointWidget to probe a
# dataset and then color the probed point (represented as a cone) as
# per the probed value and orient it as per the vector.

import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Start by loading some data.
pl3d = vtk.vtkMultiBlockPLOT3DReader()
pl3d.SetXYZFileName(VTK_DATA_ROOT + "/Data/combxyz.bin")
pl3d.SetQFileName(VTK_DATA_ROOT + "/Data/combq.bin")
pl3d.SetScalarFunctionNumber(100)
pl3d.SetVectorFunctionNumber(202)
pl3d.Update()
pl3d_output = pl3d.GetOutput().GetBlock(0)

# The plane widget is used probe the dataset.
pointWidget = vtk.vtkPointWidget()
pointWidget.SetInputData(pl3d_output)
pointWidget.AllOff()
pointWidget.PlaceWidget()
point = vtk.vtkPolyData()
pointWidget.GetPolyData(point)

probe = vtk.vtkProbeFilter()
probe.SetInputData(point)
probe.SetSourceData(pl3d_output)

# create glyph
cone = vtk.vtkConeSource()
cone.SetResolution(16)
glyph = vtk.vtkGlyph3D()
glyph.SetInputConnection(probe.GetOutputPort())
glyph.SetSourceConnection(cone.GetOutputPort())
glyph.SetVectorModeToUseVector()
glyph.SetScaleModeToDataScalingOff()
glyph.SetScaleFactor(pl3d_output.GetLength()*0.1)
glyphMapper = vtk.vtkPolyDataMapper()
glyphMapper.SetInputConnection(glyph.GetOutputPort())
glyphActor = vtk.vtkActor()
glyphActor.SetMapper(glyphMapper)
glyphActor.VisibilityOff()

# An outline is shown for context.
outline = vtk.vtkStructuredGridOutlineFilter()
outline.SetInputData(pl3d_output)
outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = vtk.vtkActor()
outlineActor.SetMapper(outlineMapper)

# Create the RenderWindow, Renderer and both Actors
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Actually set the color and orientation of the probe.
def BeginInteraction(obj, event):
    global point, glyphActor
    obj.GetPolyData(point)
    glyphActor.VisibilityOn()

def ProbeData(obj, event):
    obj.GetPolyData(point)

# Associate the line widget with the interactor
pointWidget.SetInteractor(iren)
pointWidget.AddObserver("EnableEvent", BeginInteraction)
pointWidget.AddObserver("StartInteractionEvent", BeginInteraction)
pointWidget.AddObserver("InteractionEvent", ProbeData)

# Add the actors to the renderer, set the background and size
ren.AddActor(outlineActor)
ren.AddActor(glyphActor)

ren.SetBackground(1, 1, 1)
renWin.SetSize(300, 300)
ren.SetBackground(0.1, 0.2, 0.4)

cam1 = ren.GetActiveCamera()
cam1.SetClippingRange(3.95297, 50)
cam1.SetFocalPoint(9.71821, 0.458166, 29.3999)
cam1.SetPosition(2.7439, -37.3196, 38.7167)
cam1.SetViewUp(-0.16123, 0.264271, 0.950876)

iren.Initialize()
renWin.Render()
iren.Start()
