catch {load vtktcl}
# Generate geometry for Structured grid of each dimension
# get the interactor ui
source ../../examplesTcl/vtkInt.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1

vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkMath math

# create a 0, 1, 2 and 3 dimensional Structured frid
#
set dimensions(0) "1 1 1"
set dimensions(1) "13 1 1"
set dimensions(2) "13 11 1"
set dimensions(3) "13 11 11"
set dims "0 1 2 3"
set array(0) vtkFloatArray
set array(1) vtkDoubleArray
set array(2) vtkFloatArray
set array(3) vtkDoubleArray
foreach dim $dims {
    set numTuples [expr [lindex $dimensions($dim) 0] * [lindex $dimensions($dim) 1] * [lindex $dimensions($dim) 2]]
  vtkPoints points$dim
    points$dim SetNumberOfPoints $numTuples
  
  set rMin 0.5
  set rMax 1.0
  set deltaZ 0.0
  catch {set deltaZ [expr 2.0 / ([lindex $dimensions($dim) 2]-1)]}
  set deltaRad 0.0
  catch {set deltaRad [expr ($rMax-$rMin) / ([lindex $dimensions($dim) 1]-1)]}

  for {set k 0} {$k<[lindex $dimensions($dim) 2]} {incr k} {
    set xyz(2) [expr -1.0 + $k*$deltaZ]
    set kOffset [expr $k * [lindex $dimensions($dim) 0] * [lindex $dimensions($dim) 1]]
    for {set j 0} { $j < [lindex $dimensions($dim) 1] } {incr j} {
      set radius [expr $rMin + $j*$deltaRad]
      set jOffset [expr $j * [lindex $dimensions($dim) 0]]
      for {set i 0} {$i < [lindex $dimensions($dim) 0]} {incr i} {
        set theta [expr $i * 15.0 * [math DegreesToRadians]]
        set xyz(0) [expr $radius * cos($theta)]
        set xyz(1) [expr $radius * sin($theta)]
        set offset [expr $i + $jOffset + $kOffset]
        points$dim InsertPoint $offset $xyz(0) $xyz(1) $xyz(2)
      }
    }
  }
  # build an array of scalar values
  $array($dim) da$dim
    da$dim SetNumberOfTuples $numTuples
  for {set i 0} {$i < $numTuples} {incr i} {
    da$dim InsertComponent $i 0 [math Random 0 127]
  }
  vtkScalars s$dim
    s$dim SetData da$dim
  # define the structured grid
  vtkStructuredGrid sg$dim
  eval  sg$dim SetDimensions $dimensions($dim)
    [sg$dim GetCellData] SetScalars s$dim
    sg$dim SetPoints points$dim

  vtkStructuredGridGeometryFilter sggf$dim
    sggf$dim SetInput sg$dim
  vtkPolyDataMapper pdm$dim
    pdm$dim SetInput [sggf$dim GetOutput]
    pdm$dim SetScalarRange 0 127
  vtkActor actor$dim
    actor$dim SetMapper pdm$dim
    actor$dim AddPosition [expr 2.0 * $dim] 0  0
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
iren Initialize
renWin SetFileName "StructuredGridGeometry.tcl.ppm"
#renWin SaveImageAsPPM

vtkDataSetWriter writer
  writer SetFileName sgrid.vtk
  writer SetInput sg3
  writer Update

proc TkCheckAbort {} {
  set foo [renWin GetEventPending]
  if {$foo != 0} {renWin SetAbortRender 1}
}
renWin SetAbortCheckMethod {TkCheckAbort}

# prevent the tk window from showing up then start the event loop
wm withdraw .


