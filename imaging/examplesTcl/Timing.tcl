catch {load vtktcl}
# Simple viewer for images.

source vtkImageInclude.tcl

# Image pipeline

vtkImageVolume16Reader reader
reader ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataDimensions 256 256 1
reader SetFilePrefix "../../../data/fullHead/headsq"
reader SetDataMask 0x7fff
#reader DebugOn
#reader Update


vtkImageViewer viewer
#viewer SetAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Z_AXIS $VTK_IMAGE_Y_AXIS
viewer SetInput [reader GetOutput]
viewer SetCoordinate2 0
viewer SetColorWindow 2000
viewer SetColorLevel 1000
viewer SetOriginLocationToUpperLeft
#viewer DebugOn
viewer Render

wm withdraw .


# time the window level operation
set i 0;
proc timeit {} {
  global i
  puts [expr 1000000.0/[lindex [time {viewer SetColorLevel $i; viewer Render; incr i} 100] 0]]
}




