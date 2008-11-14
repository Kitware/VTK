package require vtk
package require vtkinteraction

vtkRenderer ren1
vtkRenderWindow renWin
  renWin SetMultiSamples 0
  renWin AddRenderer ren1
vtkRenderWindowInteractor iren
  iren SetRenderWindow renWin

  # read data
vtkPLOT3DReader reader
  reader SetXYZFileName "$VTK_DATA_ROOT/Data/combxyz.bin"
  reader SetQFileName "$VTK_DATA_ROOT/Data/combq.bin"
  reader SetScalarFunctionNumber 110
  reader Update

  # create outline
vtkStructuredGridOutlineFilter outlineF
  outlineF SetInputConnection [reader GetOutputPort]
vtkPolyDataMapper outlineMapper
  outlineMapper SetInputConnection [outlineF GetOutputPort]
vtkActor outline
  outline SetMapper outlineMapper
  [outline GetProperty] SetColor 0 0 0

  # create cursor
vtkCursor3D cursor
  eval cursor SetModelBounds [[reader GetOutput] GetBounds]
  eval cursor SetFocalPoint [[reader GetOutput] GetCenter]
  cursor AllOff
  cursor AxesOn
  cursor OutlineOn
  cursor XShadowsOn
  cursor YShadowsOn
  cursor ZShadowsOn
vtkPolyDataMapper cursorMapper
  cursorMapper SetInputConnection [cursor GetOutputPort]
vtkActor cursorActor
  cursorActor SetMapper cursorMapper
  [cursorActor GetProperty] SetColor 1 0 0

  # create probe
vtkProbeFilter probe
  probe SetInput [cursor GetFocus]
  probe SetSource [reader GetOutput]

  # create a cone geometry for glyph
vtkConeSource cone
  cone SetResolution 16
  cone SetRadius 0.25

  # create glyph
vtkGlyph3D glyph
  glyph SetInputConnection [probe GetOutputPort]
  glyph SetSource [cone GetOutput]
  glyph SetVectorModeToUseVector
  glyph SetScaleModeToScaleByScalar
  glyph SetScaleFactor .0002
vtkPolyDataMapper glyphMapper
  glyphMapper SetInputConnection [glyph GetOutputPort]
vtkActor glyphActor
  glyphActor SetMapper glyphMapper

ren1 AddActor outline
ren1 AddActor cursorActor
ren1 AddActor glyphActor
ren1 SetBackground 1.0 1.0 1.0
renWin SetSize 200 200
ren1 ResetCamera
[ren1 GetActiveCamera] Elevation 60
ren1 ResetCameraClippingRange
renWin Render

iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .
