catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# Simple viewer for images.

source ../../imaging/examplesTcl/vtkImageInclude.tcl

# Image pipeline

vtkImageReader reader
  reader ReleaseDataFlagOff
  reader SetDataByteOrderToLittleEndian
  reader SetDataExtent 0 255 0 255 1 93
  reader SetDataOrigin -127.5 -127.5 -47
  reader SetFilePrefix "$VTK_DATA/fullHead/headsq"
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
  reslice OptimizationOff
  reslice SetBackgroundLevel 1023
  reslice AutoCropOutputOn

vtkImageViewer viewer
  viewer SetInput [reslice GetOutput]
  viewer SetZSlice 120
  viewer SetColorWindow 2000
  viewer SetColorLevel 1000
  viewer Render

source ../../imaging/examplesTcl/WindowLevelInterface.tcl

vtkWindowToImageFilter windowToimage
  windowToimage SetInput [viewer GetImageWindow]
vtkPNMWriter pnmWriter
  pnmWriter SetInput [windowToimage GetOutput]
  pnmWriter SetFileName "TestResliceOptimizeOff.tcl.ppm"
#  pnmWriter Write

