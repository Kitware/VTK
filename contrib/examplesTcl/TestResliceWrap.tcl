catch {load vtktcl}
# Simple viewer for images.

source ../../imaging/examplesTcl/vtkImageInclude.tcl

# Image pipeline

vtkPNMReader reader
  reader SetFileName "../../../vtkdata/masonry.ppm"
  reader SetDataExtent 0 255 0 255 0 0
  reader SetDataSpacing 1 1 1
  reader SetDataOrigin 0 0 0
  reader UpdateWholeExtent

vtkTransform transform
  transform RotateZ 45
  transform Translate 0 0 0
  transform Scale 1.414 1.414 1.414

vtkImageReslice reslice
  reslice SetInput [reader GetOutput]
  reslice SetResliceTransform transform
  reslice InterpolateOn
  reslice SetInterpolationModeToCubic
  reslice WrapOn

vtkImageViewer viewer
  viewer SetInput [reslice GetOutput]
  viewer SetZSlice 0
  viewer SetColorWindow 256
  viewer SetColorLevel 127.5
  viewer Render

source ../../imaging/examplesTcl/WindowLevelInterface.tcl

vtkWindowToImageFilter windowToimage
  windowToimage SetInput [viewer GetImageWindow]
vtkPNMWriter pnmWriter
  pnmWriter SetInput [windowToimage GetOutput]
  pnmWriter SetFileName "TestResliceWrap.tcl.ppm"
#  pnmWriter Write

