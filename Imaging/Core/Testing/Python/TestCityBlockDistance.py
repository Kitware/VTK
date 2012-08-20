#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# A script to test the threshold filter.
# Values above 2000 are set to 255.
# Values below 2000 are set to 0.
# Image pipeline
reader = vtk.vtkPNGReader()
reader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/fullhead15.png")
cast = vtk.vtkImageCast()
cast.SetOutputScalarTypeToShort()
cast.SetInputConnection(reader.GetOutputPort())
thresh = vtk.vtkImageThreshold()
thresh.SetInputConnection(cast.GetOutputPort())
thresh.ThresholdByUpper(2000.0)
thresh.SetInValue(0)
thresh.SetOutValue(200)
thresh.ReleaseDataFlagOff()
dist = vtk.vtkImageCityBlockDistance()
dist.SetDimensionality(2)
dist.SetInputConnection(thresh.GetOutputPort())
viewer = vtk.vtkImageViewer()
viewer.SetInputConnection(dist.GetOutputPort())
viewer.SetColorWindow(117)
viewer.SetColorLevel(43)
viewer.Render()
# --- end of script --
