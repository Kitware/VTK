catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# Simple viewer for images.

source vtkImageInclude.tcl

# Image pipeline

vtkImageReader reader
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 2
reader SetFilePrefix "$VTK_DATA/fullHead/headsq"
reader SetDataMask 0x7fff
#reader DebugOn

vtkImageMagnify magnify
magnify SetInput [reader GetOutput]
magnify SetMagnificationFactors 2 2 1
magnify InterpolateOn

vtkImageViewer viewer
viewer SetInput [magnify GetOutput]
viewer SetZSlice 1
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





