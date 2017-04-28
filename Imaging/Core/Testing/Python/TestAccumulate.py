#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Image pipeline
reader = vtk.vtkPNGReader()
reader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/fullhead15.png")
smooth = vtk.vtkImageGaussianSmooth()
smooth.SetDimensionality(2)
smooth.SetStandardDeviations(1,1)
smooth.SetInputConnection(reader.GetOutputPort())
imageAppend = vtk.vtkImageAppendComponents()
imageAppend.AddInputConnection(reader.GetOutputPort())
imageAppend.AddInputConnection(smooth.GetOutputPort())
clip = vtk.vtkImageClip()
clip.SetInputConnection(imageAppend.GetOutputPort())
clip.SetOutputWholeExtent(0,255,0,255,20,22)
accum = vtk.vtkImageAccumulate()
accum.SetInputConnection(clip.GetOutputPort())
accum.SetComponentExtent(0,255,0,255,0,0)
accum.SetComponentSpacing(12,12,0.0)
viewer = vtk.vtkImageViewer()
viewer.SetInputConnection(accum.GetOutputPort())
viewer.SetColorWindow(4)
viewer.SetColorLevel(2)
viewer.Render()
# --- end of script --
