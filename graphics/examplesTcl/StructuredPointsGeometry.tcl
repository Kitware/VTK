catch {load vtktcl}
# Generate geometry for structured points of each dimension
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

# create a 0, 1, 2 and 3 dimensional structured points
#
set dimensions(0) "1 1 1"
set dimensions(1) "10 1 1"
set dimensions(2) "10 10 1"
set dimensions(3) "10 10 10"
set dims "0 1 2 3"
set array(0) vtkUnsignedCharArray
set array(1) vtkShortArray
set array(2) vtkLongArray
set array(3) vtkDoubleArray
foreach dim $dims {
    set numTuples [expr [lindex $dimensions($dim) 0] * [lindex $dimensions($dim) 1] * [lindex $dimensions($dim) 2]]
  $array($dim) da$dim
      da$dim SetNumberOfTuples $numTuples
  for {set i 0} {$i < $numTuples} {incr i} {
    da$dim InsertComponent $i 0 [math Random -100 100]
  }
  vtkScalars s$dim
    s$dim SetData da$dim
  vtkStructuredPoints sp$dim
  eval  sp$dim SetDimensions $dimensions($dim)
    [sp$dim GetCellData] SetScalars s$dim
  vtkStructuredPointsGeometryFilter spgf$dim
    spgf$dim SetInput sp$dim
  vtkPolyDataMapper pdm$dim
    pdm$dim SetInput [spgf$dim GetOutput]
    pdm$dim SetScalarRange -100 100
  vtkActor actor$dim
    actor$dim SetMapper pdm$dim
    actor$dim SetPosition [expr $dim * 10] 0 0
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
#renWin SetFileName "StructuredPointsGeometry.tcl.ppm"
#renWin SaveImageAsPPM
vtkStructuredPointsWriter writer
  writer SetFileName sp.vtk
  writer SetInput sp3
  writer Update

proc TkCheckAbort {} {
  set foo [renWin GetEventPending]
  if {$foo != 0} {renWin SetAbortRender 1}
}
renWin SetAbortCheckMethod {TkCheckAbort}

# prevent the tk window from showing up then start the event loop
wm withdraw .


