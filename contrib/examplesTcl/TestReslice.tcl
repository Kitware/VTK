catch {load vtktcl}
# Simple viewer for images.

source ../../imaging/examplesTcl/vtkImageInclude.tcl

# Image pipeline

vtkImageReader reader
  reader ReleaseDataFlagOff
  reader SetDataByteOrderToLittleEndian
  reader SetDataExtent 0 255 0 255 1 93
  reader SetDataOrigin -127.5 -127.5 -47
  reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
  reader SetDataMask 0x7fff
  reader UpdateWholeExtent

vtkTransform transform
  transform RotateX 10
  transform RotateY 20
  transform RotateZ 30

vtkImageReslice reslice
  reslice SetInput [reader GetOutput]
  reslice SetResliceTransform transform
  reslice InterpolateOn
  reslice SetBackgroundLevel 1023

vtkImageViewer viewer
  viewer SetInput [reslice GetOutput]
  viewer SetZSlice 50
  viewer SetColorWindow 2000
  viewer SetColorLevel 1000
  viewer Render

source ../../imaging/examplesTcl/WindowLevelInterface.tcl

vtkWindowToImageFilter windowToimage
  windowToimage SetInput [viewer GetImageWindow]
vtkPNMWriter pnmWriter
  pnmWriter SetInput [windowToimage GetOutput]
  pnmWriter SetFileName "TestReslice.tcl.ppm"
#  pnmWriter Write

