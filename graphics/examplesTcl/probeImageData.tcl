catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# get the interactor ui
source $VTK_TCL/vtkInt.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderer ren2
vtkRenderWindow renWin
  renWin AddRenderer ren1
  renWin AddRenderer ren2
vtkRenderWindowInteractor iren
  iren SetRenderWindow renWin

# create pipeline
#
vtkImageReader reader
  reader SetDataByteOrderToLittleEndian
  reader SetDataExtent 0 255 0 255 1 93
  reader SetDataVOI 20 200 20 200 40 40
  reader SetFilePrefix "$VTK_DATA/fullHead/headsq"
  reader SetDataMask 0x7fff
  reader Update

vtkLineSource probeLine
  probeLine SetPoint1 25 25 39
  probeLine SetPoint2 150 150 39
  probeLine SetPoint1 10 10 39
  probeLine SetPoint2 210 210  39
  probeLine SetResolution 100

vtkProbeFilter probe
  probe SetInput [probeLine GetOutput]
  probe SetSource [reader GetOutput]

vtkTubeFilter probeTube
  probeTube SetInput [probe GetPolyDataOutput]
  probeTube SetNumberOfSides 5
  probeTube SetRadius 1

vtkPolyDataMapper probeMapper
  probeMapper SetInput [probeTube GetOutput]
  eval probeMapper SetScalarRange [[reader GetOutput] GetScalarRange]

vtkActor probeActor
  probeActor SetMapper probeMapper

vtkLineSource displayLine
  displayLine SetPoint1 0 0 0
  displayLine SetPoint2 1 0 0
  displayLine SetResolution [probeLine GetResolution]

vtkMergeFilter displayMerge
  displayMerge SetGeometry [displayLine GetOutput]
  displayMerge SetScalars [probe GetPolyDataOutput]

vtkWarpScalar displayWarp
  displayWarp SetInput [displayMerge GetPolyDataOutput]
  displayWarp SetNormal 0 1 0
  displayWarp SetScaleFactor .00005

vtkPolyDataMapper displayMapper
  displayMapper SetInput [displayWarp GetPolyDataOutput]
eval displayMapper SetScalarRange [[reader GetOutput] GetScalarRange]

vtkActor displayActor
  displayActor SetMapper displayMapper

vtkOutlineFilter outline
  outline SetInput [reader GetOutput]
vtkPolyDataMapper outlineMapper
  outlineMapper SetInput [outline GetOutput]
vtkActor outlineActor
  outlineActor SetMapper outlineMapper
  [outlineActor GetProperty] SetColor 0 0 0

ren1 AddActor outlineActor
ren1 AddActor probeActor
ren1 SetBackground 1 1 1
ren1 SetViewport 0 .25 1 1

ren2 AddActor displayActor
ren2 SetBackground 0 0 0
ren2 SetViewport 0 0 1 .25

renWin SetSize 500 500


set cam2 [ren2 GetActiveCamera]
$cam2 ParallelProjectionOn
$cam2 SetParallelScale .15

iren Initialize

renWin SetFileName "probeImageData.tcl.ppm"
#renWin SaveImageAsPPM

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .



