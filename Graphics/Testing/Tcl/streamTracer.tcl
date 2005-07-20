package require vtk
package require vtkinteraction

vtkRenderer ren1
vtkRenderWindow renWin
renWin AddRenderer ren1
vtkRenderWindowInteractor iren
iren SetRenderWindow renWin

# read data
#
vtkStructuredGridReader reader
reader SetFileName "$VTK_DATA_ROOT/Data/office.binary.vtk"
reader Update;#force a read to occur

vtkStructuredGridOutlineFilter outline
outline SetInputConnection [reader GetOutputPort]
vtkPolyDataMapper mapOutline
mapOutline SetInputConnection [outline GetOutputPort]
vtkActor outlineActor
outlineActor SetMapper mapOutline
[outlineActor GetProperty] SetColor 0 0 0

vtkRungeKutta45 rk

# Create source for streamtubes
vtkStreamTracer streamer
streamer SetInputConnection [reader GetOutputPort]
streamer SetStartPosition 0.1 2.1 0.5
streamer SetMaximumPropagation 0 500
streamer SetMinimumIntegrationStep 1 0.1
streamer SetMaximumIntegrationStep 1 1.0
streamer SetInitialIntegrationStep 2 0.2
streamer SetIntegrationDirection 0
streamer SetIntegrator rk
streamer SetRotationScale 0.5
streamer SetMaximumError 1.0e-8

vtkAssignAttribute aa
aa SetInputConnection [streamer GetOutputPort]
aa Assign "Normals" "NORMALS" "POINT_DATA"

vtkRibbonFilter rf1
rf1 SetInputConnection [aa GetOutputPort]
rf1 SetWidth 0.1
rf1 VaryWidthOff

vtkPolyDataMapper mapStream
mapStream SetInputConnection [rf1 GetOutputPort]
eval mapStream SetScalarRange [[reader GetOutput] GetScalarRange]
vtkActor streamActor
streamActor SetMapper mapStream

ren1 AddActor outlineActor
ren1 AddActor streamActor

ren1 SetBackground 0.4 0.4 0.5

set cam [ren1 GetActiveCamera] 
$cam SetPosition -2.35599 -3.35001 4.59236 
$cam SetFocalPoint 2.255 2.255 1.28413 
$cam SetViewUp 0.311311 0.279912 0.908149
$cam SetClippingRange 1.12294 16.6226 

renWin SetSize 300 200
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

# interact with data
wm withdraw .
