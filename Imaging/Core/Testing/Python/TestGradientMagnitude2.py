#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Image pipeline
reader = vtk.vtkPNGReader()
reader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/fullhead15.png")
clip = vtk.vtkImageClip()
clip.SetInputConnection(reader.GetOutputPort())
clip.SetOutputWholeExtent(80,230,80,230,0,0)
clip.ClipDataOff()
gradient = vtk.vtkImageGradientMagnitude()
gradient.SetDimensionality(2)
gradient.SetInputConnection(clip.GetOutputPort())
gradient.HandleBoundariesOff()
slide = vtk.vtkImageChangeInformation()
slide.SetInputConnection(gradient.GetOutputPort())
slide.SetExtentTranslation(-100,-100,0)
viewer = vtk.vtkImageViewer()
viewer.SetInputConnection(slide.GetOutputPort())
viewer.SetColorWindow(-1000)
viewer.SetColorLevel(500)
viewer.Render()
#skipping source
# --- end of script --
