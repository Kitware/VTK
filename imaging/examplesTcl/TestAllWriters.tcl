catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# This script reads an image and writes it in several formats

source vtkImageInclude.tcl

# Image pipeline

vtkBMPReader image
  image SetFileName "$VTK_DATA/beach.bmp"

vtkImageLuminance luminance
  luminance SetInput [image GetOutput]

vtkTIFFWriter tiff1
  tiff1 SetInput [image GetOutput]
  tiff1 SetFileName tiff1.tif

vtkTIFFWriter tiff2
  tiff2 SetInput [luminance GetOutput]
  tiff2 SetFileName tiff2.tif

vtkBMPWriter bmp1
  bmp1 SetInput [image GetOutput]
  bmp1 SetFileName bmp1.bmp

vtkBMPWriter bmp2
  bmp2 SetInput [luminance GetOutput]
  bmp2 SetFileName bmp2.bmp

tiff1 Write
tiff2 Write
bmp1 Write
bmp2 Write

vtkImageViewer viewer
  viewer SetInput [luminance GetOutput]
  viewer SetColorWindow 255
  viewer SetColorLevel 127.5
  viewer Render

#make interface
source WindowLevelInterface.tcl

vtkWindowToImageFilter windowToimage
  windowToimage SetInput [viewer GetImageWindow]
vtkPNMWriter pnmWriter
  pnmWriter SetInput [windowToimage GetOutput]
  pnmWriter SetFileName "TestAllWriters.tcl.ppm"
#  pnmWriter Write
