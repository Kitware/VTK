#!/usr/bin/env python

# This example demonstrates cell picking using vtkCellPicker.  It
# displays the results of picking using a vtkTextMapper.

import vtk

# create a sphere source, mapper, and actor
sphere = vtk.vtkSphereSource()
sphereMapper = vtk.vtkPolyDataMapper()
sphereMapper.SetInputConnection(sphere.GetOutputPort())
sphereMapper.GlobalImmediateModeRenderingOn()
sphereActor = vtk.vtkLODActor()
sphereActor.SetMapper(sphereMapper)

# create the spikes by glyphing the sphere with a cone.  Create the
# mapper and actor for the glyphs.
cone = vtk.vtkConeSource()
glyph = vtk.vtkGlyph3D()
glyph.SetInputConnection(sphere.GetOutputPort())
glyph.SetSource(cone.GetOutput())
glyph.SetVectorModeToUseNormal()
glyph.SetScaleModeToScaleByVector()
glyph.SetScaleFactor(0.25)
spikeMapper = vtk.vtkPolyDataMapper()
spikeMapper.SetInputConnection(glyph.GetOutputPort())
spikeActor = vtk.vtkLODActor()
spikeActor.SetMapper(spikeMapper)

# Create a text mapper and actor to display the results of picking.
textMapper = vtk.vtkTextMapper()
tprop = textMapper.GetTextProperty()
tprop.SetFontFamilyToArial()
tprop.SetFontSize(10)
tprop.BoldOn()
tprop.ShadowOn()
tprop.SetColor(1, 0, 0)
textActor = vtk.vtkActor2D()
textActor.VisibilityOff()
textActor.SetMapper(textMapper)

# Create a cell picker.
picker = vtk.vtkCellPicker()

# Create a Python function to create the text for the text mapper used
# to display the results of picking.
def annotatePick(object, event):
    global picker, textActor, textMapper
    if picker.GetCellId() < 0:
        textActor.VisibilityOff()
    else:
        selPt = picker.GetSelectionPoint()
        pickPos = picker.GetPickPosition()
        textMapper.SetInput("(%.6f, %.6f, %.6f)"%pickPos)
        textActor.SetPosition(selPt[:2])
        textActor.VisibilityOn()
     
# Now at the end of the pick event call the above function.
picker.AddObserver("EndPickEvent", annotatePick)

# Create the Renderer, RenderWindow, etc. and set the Picker.
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
iren.SetPicker(picker)

# Add the actors to the renderer, set the background and size
ren.AddActor2D(textActor)
ren.AddActor(sphereActor)
ren.AddActor(spikeActor)
ren.SetBackground(1, 1, 1)
renWin.SetSize(300, 300)

# Get the camera and zoom in closer to the image.
ren.ResetCamera()
cam1 = ren.GetActiveCamera()
cam1.Zoom(1.4)

iren.Initialize()
# Initially pick the cell at this location.
picker.Pick(85, 126, 0, ren)
renWin.Render()
iren.Start()
