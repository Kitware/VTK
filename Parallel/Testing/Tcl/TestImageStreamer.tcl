package require vtk

vtkImageReader reader
reader ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 63 0 63 1 93
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

proc changeLUT { } {
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
mapToRGBA SetInput [reader GetOutput]
mapToRGBA SetOutputFormatToRGBA
mapToRGBA SetLookupTable LUT
mapToRGBA AddObserver EndEvent changeLUT

vtkMemoryLimitImageDataStreamer streamer
streamer SetInput [mapToRGBA GetOutput]
streamer SetMemoryLimit 100
streamer UpdateWholeExtent

# set the window/level to 255.0/127.5 to view full range
vtkImageViewer viewer
viewer SetInput [streamer GetOutput]
viewer SetColorWindow 255.0
viewer SetColorLevel 127.5
viewer SetZSlice 50

wm withdraw .
viewer Render

