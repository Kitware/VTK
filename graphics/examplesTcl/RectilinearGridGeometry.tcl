catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# Generate geometry for rectilinear grid of each dimension
# get the interactor ui
source $VTK_TCL/vtkInt.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1

vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkMath math

# create a 0, 1, 2 and 3 dimensional rectilinear frid
#
vtkFloatArray sxArray0
 sxArray0 InsertNextValue 0
vtkFloatArray syArray0
 syArray0 InsertNextValue 0
vtkFloatArray szArray0
 szArray0 InsertNextValue 0

vtkFloatArray sxArray1
set j 0
for {set i 0} {$i < 10} {incr i} {
 sxArray1 InsertNextValue $j
 set j [expr $j + $i + 1]
}
vtkFloatArray syArray1
 syArray1 InsertNextValue 0
vtkFloatArray szArray1
 szArray1 InsertNextValue 0

vtkFloatArray sxArray2
set j 0
for {set i 0} {$i < 10} {incr i} {
 sxArray2 InsertNextValue $j
 set j [expr $j + $i + 1]
}
vtkFloatArray syArray2
set j 0
for {set i 0} {$i < 10} {incr i} {
 syArray2 InsertNextValue $j
 set j [expr $j + $i + 1]
}
vtkFloatArray szArray2
 szArray2 InsertNextValue 0

vtkFloatArray sxArray3
set j 0
for {set i 0} {$i < 10} {incr i} {
 sxArray3 InsertNextValue $j
 set j [expr $j + $i + 1]
}
vtkFloatArray syArray3
set j 0
for {set i 0} {$i < 10} {incr i} {
 syArray3 InsertNextValue $j
 set j [expr $j + $i + 1]
}
vtkFloatArray szArray3
set j 0
for {set i 0} {$i < 10} {incr i} {
 szArray3 InsertNextValue $j
 set j [expr $j + $i + 1]
}


set dimensions(0) "1 1 1"
set dimensions(1) "10 1 1"
set dimensions(2) "10 10 1"
set dimensions(3) "10 10 10"
set dims "0 1 2 3"
set array(0) vtkUnsignedCharArray
set array(1) vtkUnsignedShortArray
set array(2) vtkUnsignedLongArray
set array(3) vtkFloatArray
foreach dim $dims {
    set numTuples [expr [lindex $dimensions($dim) 0] * [lindex $dimensions($dim) 1] * [lindex $dimensions($dim) 2]]
  $array($dim) da$dim
      da$dim SetNumberOfTuples $numTuples
  for {set i 0} {$i < $numTuples} {incr i} {
    da$dim InsertComponent $i 0 [math Random 0 127]
  }
  vtkRectilinearGrid rg$dim
  eval  rg$dim SetDimensions $dimensions($dim)
    [rg$dim GetCellData] SetScalars da$dim
    rg$dim SetXCoordinates sxArray$dim
    rg$dim SetYCoordinates syArray$dim
    rg$dim SetZCoordinates szArray$dim
  vtkRectilinearGridGeometryFilter rggf$dim
    rggf$dim SetInput rg$dim
  vtkPolyDataMapper pdm$dim
    pdm$dim SetInput [rggf$dim GetOutput]
    pdm$dim SetScalarRange 0 127
  vtkActor actor$dim
    actor$dim SetMapper pdm$dim
    actor$dim AddPosition [expr 50 * $dim] 0  0
  ren1 AddActor actor$dim
}

ren1 SetBackground 0.2 0.2 0.2
renWin SetSize 300 150

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

set cam1 [ren1 GetActiveCamera]
$cam1 Azimuth -30
$cam1 Elevation 30
$cam1 Zoom 2.5
ren1 ResetCameraClippingRange
renWin Render

#renWin SetFileName "RectilinearGridGeometry.tcl.ppm"
#renWin SaveImageAsPPM
vtkDataSetWriter writer
  writer SetFileName rgrid.vtk
  writer SetInput rg3
  writer Update

proc TkCheckAbort {} {
  set foo [renWin GetEventPending]
  if {$foo != 0} {renWin SetAbortRender 1}
}
renWin SetAbortCheckMethod {TkCheckAbort}

# prevent the tk window from showing up then start the event loop
wm withdraw .


