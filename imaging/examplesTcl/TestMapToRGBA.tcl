catch {load vtktcl}

source vtkImageInclude.tcl

vtkImageReader reader
reader ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff

vtkLookupTable LUT 
LUT SetTableRange 0 1800
LUT SetSaturationRange 1 1
LUT SetHueRange 0 1
LUT SetValueRange 1 1  
LUT SetAlphaRange 0 0
LUT Build

vtkImageMapToRGBA mapToRGBA
mapToRGBA SetInput [reader GetOutput]
mapToRGBA SetLookupTable LUT

# set the window/level to 255.0/127.5 to view full range
vtkImageViewer viewer
viewer SetInput [mapToRGBA GetOutput]
viewer SetColorWindow 255.0
viewer SetColorLevel 127.5
viewer SetZSlice 50

viewer Render

#make interface
source WindowLevelInterface.tcl

vtkWindowToImageFilter windowToimage
  windowToimage SetInput [viewer GetImageWindow]

vtkPNMWriter pnmWriter
  pnmWriter SetInput [windowToimage GetOutput]
  pnmWriter SetFileName "TestMapToRGBA.tcl.ppm"
#  pnmWriter Write

