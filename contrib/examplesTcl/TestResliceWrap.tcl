catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# Simple viewer for images.

source ../../imaging/examplesTcl/vtkImageInclude.tcl

# Image pipeline

vtkPNMReader reader
  reader SetFileName "$VTK_DATA/masonry.ppm"
  reader SetDataExtent 0 255 0 255 0 0
  reader SetDataSpacing 1 1 1
  reader SetDataOrigin 0 0 0
  reader UpdateWholeExtent

vtkTransform transform
  transform RotateZ 45
  transform Scale 1.414 1.414 1.414

vtkImageReslice reslice
  reslice SetInput [reader GetOutput]
  reslice SetResliceTransform transform
  reslice InterpolateOn
  reslice SetInterpolationModeToCubic
  reslice WrapOn
  reslice AutoCropOutputOn

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

