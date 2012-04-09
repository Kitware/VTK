package require vtk
package require vtkinteraction
package require vtktesting

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create pipeline
#
vtkVolume16Reader v16
  v16 SetDataDimensions 64 64
  [v16 GetOutput] SetOrigin 0.0 0.0 0.0
  v16 SetDataByteOrderToLittleEndian
  v16 SetFilePrefix "$VTK_DATA_ROOT/Data/headsq/quarter"
  v16 SetImageRange 1 93
  v16 SetDataSpacing 3.2 3.2 1.5
  v16 Update

vtkImageMarchingCubes iso
  iso SetInputConnection [v16 GetOutputPort]
  iso SetValue 0 1150
  iso SetInputMemoryLimit 1000

vtkPolyDataMapper isoMapper
  isoMapper SetInputConnection [iso GetOutputPort]
  isoMapper ScalarVisibilityOff
  isoMapper ImmediateModeRenderingOn

vtkActor isoActor
  isoActor SetMapper isoMapper
  eval [isoActor GetProperty] SetColor $antique_white

vtkOutlineFilter outline
  outline SetInputConnection [v16 GetOutputPort]
vtkPolyDataMapper outlineMapper
  outlineMapper SetInputConnection [outline GetOutputPort]
vtkActor outlineActor
  outlineActor SetMapper outlineMapper
  outlineActor VisibilityOff

# Add the actors to the renderer, set the background and size
#
ren1 AddActor outlineActor
ren1 AddActor isoActor
ren1 SetBackground 0.2 0.3 0.4
renWin SetSize 200 200
ren1 ResetCamera
[ren1 GetActiveCamera] Elevation 90
[ren1 GetActiveCamera] SetViewUp 0 0 -1
[ren1 GetActiveCamera] Azimuth 180
ren1 ResetCameraClippingRange

renWin Render

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .


