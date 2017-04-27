#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# A script to test the island removal filter.
# first the image is thresholded, then small islands are removed.
# Image pipeline
reader = vtk.vtkPNGReader()
reader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/fullhead15.png")
thresh = vtk.vtkImageThreshold()
thresh.SetInputConnection(reader.GetOutputPort())
thresh.ThresholdByUpper(2000.0)
thresh.SetInValue(255)
thresh.SetOutValue(0)
thresh.ReleaseDataFlagOff()
island = vtk.vtkImageIslandRemoval2D()
island.SetInputConnection(thresh.GetOutputPort())
island.SetIslandValue(255)
island.SetReplaceValue(128)
island.SetAreaThreshold(100)
island.SquareNeighborhoodOn()
viewer = vtk.vtkImageViewer()
viewer.SetInputConnection(island.GetOutputPort())
viewer.SetColorWindow(255)
viewer.SetColorLevel(127.5)
#viewer DebugOn
viewer.Render()
# --- end of script --
