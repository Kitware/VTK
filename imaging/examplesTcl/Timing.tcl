catch {load vtktcl}
# Simple viewer for images.

source vtkImageInclude.tcl

# Image pipeline

vtkImageReader reader
reader ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 2
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff
#reader DebugOn
#reader Update

vtkImageMagnify magnify
magnify SetDimensionality 2
magnify SetInput [reader GetOutput]
magnify SetMagnificationFactors 2 2
magnify InterpolateOn
magnify ReleaseDataFlagOff

vtkImageViewer viewer
#viewer SetAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Z_AXIS $VTK_IMAGE_Y_AXIS
#viewer SetInput [reader GetOutput]
viewer SetInput [magnify GetOutput]
viewer SetCoordinate2 1
viewer SetColorWindow 2000
viewer SetColorLevel 1000
#viewer DebugOn
viewer Render

wm withdraw .


# time the window level operation
set i 0;
proc timeit {} {
  global i
   puts start
   puts [expr 1000000.0/[lindex [time {viewer SetColorLevel $i; viewer Render; incr i} 100] 0]]
   puts end
}

timeit





