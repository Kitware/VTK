catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }


source vtkImageInclude.tcl

vtkImageReader reader
reader ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "$VTK_DATA/fullHead/headsq"
reader SetDataMask 0x7fff

vtkLookupTable LUT 
LUT SetTableRange 0 1800
LUT SetSaturationRange 1 1
LUT SetHueRange 0 1
LUT SetValueRange 1 1  
LUT SetAlphaRange 0 0
LUT Build

vtkImageMapToWindowLevelColors mapToWL
mapToWL SetInput [reader GetOutput]
mapToWL SetWindow 2000
mapToWL SetLevel 1000

# set the window/level to 255.0/127.5 to view full range
vtkImageViewer viewer
viewer SetInput [mapToWL GetOutput]
viewer SetColorWindow 255.0
viewer SetColorLevel 127.5
viewer SetZSlice 50

viewer Render

#make interface
source WindowLevelInterface.tcl

vtkWindowToImageFilter windowToimage
  windowToimage SetInput [viewer GetImageWindow]


