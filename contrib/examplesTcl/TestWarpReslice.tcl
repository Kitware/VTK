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
  reader SetDataSpacing 1.0 1.0 2.0
  reader SetDataOrigin -127.5 -127.5 -94
  reader SetFilePrefix "$VTK_DATA/fullHead/headsq"
  reader SetDataMask 0x7fff
  reader Update

vtkPoints p1
vtkPoints p2

p1 InsertNextPoint 0 0 0
p2 InsertNextPoint -60 10 20
p1 InsertNextPoint -128 -128 -50
p2 InsertNextPoint -128 -128 -50
p1 InsertNextPoint -128 -128 50
p2 InsertNextPoint -128 -128 50
p1 InsertNextPoint -128 128 -50
p2 InsertNextPoint -128 128 -50
p1 InsertNextPoint -128 128 50
p2 InsertNextPoint -128 128 50
p1 InsertNextPoint 128 -128 -50
p2 InsertNextPoint 128 -128 -50
p1 InsertNextPoint 128 -128 50
p2 InsertNextPoint 128 -128 50
p1 InsertNextPoint 128 128 -50
p2 InsertNextPoint 128 128 -50
p1 InsertNextPoint 128 128 50
p2 InsertNextPoint 128 128 50

vtkThinPlateSplineTransform transform
  transform SetSourceLandmarks p1
  transform SetTargetLandmarks p2
  transform SetBasisToR

vtkImageReslice reslice
  reslice SetInput [reader GetOutput]
  reslice SetResliceTransform transform
  reslice SetInterpolationModeToLinear
  reslice SetOutputSpacing 1 1 1

vtkImageCacheFilter cac
  cac SetInput [reslice GetOutput]
  cac SetCacheSize 1000

cac SetInput [reslice GetOutput]

vtkImageViewer viewer
  viewer SetInput [cac GetOutput]
  viewer SetZSlice 90
  viewer SetColorWindow 2000
  viewer SetColorLevel 1000
#  [viewer GetImageWindow] DoubleBufferOn
  viewer Render

source ../../imaging/examplesTcl/WindowLevelInterface.tcl

vtkWindowToImageFilter windowToimage
  windowToimage SetInput [viewer GetImageWindow]
vtkPNMWriter pnmWriter
  pnmWriter SetInput [windowToimage GetOutput]
  pnmWriter SetFileName "TestWarpReslice.tcl.ppm"
#  pnmWriter Write

