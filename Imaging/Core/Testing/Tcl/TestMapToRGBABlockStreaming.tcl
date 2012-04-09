package require vtk



vtkImageReader reader
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 63 0 63 1 93
reader SetDataSpacing 3.2 3.2 1.5
reader SetFilePrefix "$VTK_DATA_ROOT/Data/headsq/quarter"
reader SetDataMask 0x7fff

set rangeStart 0.0
set rangeEnd   0.2

vtkLookupTable LUT 
LUT SetTableRange 0 1800
LUT SetSaturationRange 1 1
LUT SetHueRange $rangeStart $rangeEnd
LUT SetValueRange 1 1  
LUT SetAlphaRange 1 1
LUT Build

# The prototype of this function has
# arguments so that the code can be
# translated to python for testing.
proc changeLUT { { a 0 } { b 0 } } {
    global rangeStart rangeEnd
    
    set rangeStart [expr $rangeStart + 0.1]
    set rangeEnd   [expr $rangeEnd   + 0.1]
    if { $rangeEnd > 1.0 } {
	set rangeStart 0.0
	set rangeEnd   0.2
    }

    LUT SetHueRange $rangeStart $rangeEnd
    LUT Build
}

vtkImageMapToColors mapToRGBA
mapToRGBA SetInputConnection [reader GetOutputPort]
mapToRGBA SetOutputFormatToRGBA
mapToRGBA SetLookupTable LUT
mapToRGBA AddObserver "EndEvent" changeLUT

vtkImageDataStreamer imageStreamer
imageStreamer SetInputConnection [mapToRGBA GetOutputPort]
imageStreamer SetNumberOfStreamDivisions 8
# make sure we get the correct translator.
imageStreamer UpdateInformation
[imageStreamer GetExtentTranslator] SetSplitModeToBlock

# set the window/level to 255.0/127.5 to view full range
vtkImageViewer viewer
viewer SetInputConnection [imageStreamer GetOutputPort]
viewer SetColorWindow 255.0
viewer SetColorLevel 127.5
viewer SetZSlice 50

viewer Render

#make interface
viewer Render

