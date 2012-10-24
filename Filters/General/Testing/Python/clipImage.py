#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# create pipeline
#
v16 = vtk.vtkVolume16Reader()
v16.SetDataDimensions(64,64)
v16.GetOutput().SetOrigin(0.0,0.0,0.0)
v16.SetDataByteOrderToLittleEndian()
v16.SetFilePrefix("" + str(VTK_DATA_ROOT) + "/Data/headsq/quarter")
v16.SetImageRange(45,45)
v16.SetDataSpacing(3.2,3.2,1.5)
v16.Update()
# do the pixel clipping
clip = vtk.vtkClipDataSet()
clip.SetInputConnection(v16.GetOutputPort())
clip.SetValue(1000)
clipMapper = vtk.vtkDataSetMapper()
clipMapper.SetInputConnection(clip.GetOutputPort())
clipMapper.ScalarVisibilityOff()
clipActor = vtk.vtkActor()
clipActor.SetMapper(clipMapper)
# put an outline around the data
outline = vtk.vtkOutlineFilter()
outline.SetInputConnection(v16.GetOutputPort())
outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = vtk.vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.VisibilityOff()
# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(outlineActor)
ren1.AddActor(clipActor)
ren1.SetBackground(0,0,0)
renWin.SetSize(200,200)
iren.Initialize()
# render the image
#
# prevent the tk window from showing up then start the event loop
# --- end of script --
