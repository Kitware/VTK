catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }
# Test marching cubes speed
#
vtkVolume16Reader v16
  v16 SetDataDimensions 64 64
  [v16 GetOutput] SetOrigin 0.0 0.0 0.0
  v16 SetDataByteOrderToLittleEndian
  v16 SetFilePrefix "$VTK_DATA/headsq/quarter"
  v16 SetImageRange 1 93
  v16 SetDataSpacing 3.2 3.2 1.5
  v16 Update

vtkContourFilter iso
  iso SetInput [v16 GetOutput]
  iso SetValue 0 1150

set t [format "%6.2f" [expr [lindex [time {iso Update;} 1] 0] / 1000000.0]]

puts "$t seconds"

exit
