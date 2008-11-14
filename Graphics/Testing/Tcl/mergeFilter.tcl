package require vtk
package require vtkinteraction

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderer ren2
vtkRenderWindow renWin
  renWin SetMultiSamples 0
  renWin AddRenderer ren1
  renWin AddRenderer ren2
vtkRenderWindowInteractor iren
  iren SetRenderWindow renWin

# create pipeline
#
vtkPLOT3DReader pl3d
  pl3d SetXYZFileName "$VTK_DATA_ROOT/Data/combxyz.bin"
  pl3d SetQFileName "$VTK_DATA_ROOT/Data/combq.bin"
  pl3d SetScalarFunctionNumber 110
  pl3d SetVectorFunctionNumber 202
  pl3d Update

vtkLineSource probeLine
  probeLine SetPoint1 1 1 29
  probeLine SetPoint2 16.5 5 31.7693
  probeLine SetResolution 500

vtkProbeFilter probe
  probe SetInputConnection [probeLine GetOutputPort]
  probe SetSource [pl3d GetOutput]

vtkTubeFilter probeTube
  probeTube SetInput [probe GetPolyDataOutput]
  probeTube SetNumberOfSides 5
  probeTube SetRadius .05

vtkPolyDataMapper probeMapper
  probeMapper SetInputConnection [probeTube GetOutputPort]
  eval probeMapper SetScalarRange [[pl3d GetOutput] GetScalarRange]

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
  displayWarp SetScaleFactor .000001

vtkPolyDataMapper displayMapper
  displayMapper SetInput [displayWarp GetPolyDataOutput]
eval displayMapper SetScalarRange [[pl3d GetOutput] GetScalarRange]

vtkActor displayActor
  displayActor SetMapper displayMapper

vtkStructuredGridOutlineFilter outline
  outline SetInputConnection [pl3d GetOutputPort]
vtkPolyDataMapper outlineMapper
  outlineMapper SetInputConnection [outline GetOutputPort]
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

renWin SetSize 300 300

ren1 ResetCamera
set cam1 [ren1 GetActiveCamera]
$cam1 SetClippingRange 3.95297 50
$cam1 SetFocalPoint 8.88908 0.595038 29.3342
$cam1 SetPosition 9.9 -26 41
$cam1 SetViewUp 0.060772 -0.319905 0.945498

ren2 ResetCamera
set cam2 [ren2 GetActiveCamera]
$cam2 ParallelProjectionOn
$cam2 SetParallelScale .15

iren Initialize


# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .



