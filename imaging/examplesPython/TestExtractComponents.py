#!/usr/local/bin/python

from vtkpython import *
from WindowLevelInterface import *

# Get Vectors from the gradent, and extract the z component.




# Image pipeline

#reader = vtkImageReader()
#reader.DebugOn()
#reader.SetDataByteOrderToLittleEndian()
#reader.SetDataExtent(0,255,0,255,1,93)
#reader.SetFilePrefix("../../../vtkdata/fullHead/headsq")
#reader.SetDataMask(0x7fff)

#gradient = vtkImageGradient()
#gradient.SetInput([reader.GetOutput())
#gradient.SetFilteredAxes(VTK_IMAGE_X_AXIS,VTK_IMAGE_Y_AXIS,VTK_IMAGE_Z_AXIS)

reader = vtkPNMReader()
reader.SetFileName("../../../vtkdata/masonry.ppm")

extract = vtkImageExtractComponents()
extract.SetInput(reader.GetOutput())
extract.SetComponents(0,1,2)
extract.ReleaseDataFlagOff()

viewer = vtkImageViewer()
#viewer.DebugOn()
viewer.SetInput(extract.GetOutput())
viewer.SetZSlice(0)
viewer.SetColorWindow(800)
viewer.SetColorLevel(0)

viewer.Render()

windowToimage = vtkWindowToImageFilter()
windowToimage.SetInput(viewer.GetImageWindow())

pnmWriter = vtkPNMWriter()
pnmWriter.SetInput(windowToimage.GetOutput())
pnmWriter.SetFileName("TestExtractComponents.tcl.ppm")
#pnmWriter.Write()

#make interface
WindowLevelInterface(viewer)
