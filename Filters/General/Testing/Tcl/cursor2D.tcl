package require vtk
package require vtkinteraction

  # create four cursors configured differently
vtkCursor2D cursor
  eval cursor SetModelBounds 15 45 15 45 0 0
  eval cursor SetFocalPoint 30 30 0
  cursor AllOff
  cursor AxesOn
  cursor OutlineOn
vtkPolyDataMapper2D cursorMapper
  cursorMapper SetInputConnection [cursor GetOutputPort]
vtkActor2D cursorActor
  cursorActor SetMapper cursorMapper
  [cursorActor GetProperty] SetColor 1 0 0

vtkCursor2D cursor2
  eval cursor2 SetModelBounds 75 105 15 45 0 0
  eval cursor2 SetFocalPoint 90 30 0
  cursor2 AllOff
  cursor2 AxesOn
  cursor2 OutlineOn
  cursor2 PointOn
vtkPolyDataMapper2D cursor2Mapper
  cursor2Mapper SetInputConnection [cursor2 GetOutputPort]
vtkActor2D cursor2Actor
  cursor2Actor SetMapper cursor2Mapper
  [cursor2Actor GetProperty] SetColor 0 1 0

vtkCursor2D cursor3
  eval cursor3 SetModelBounds 15 45 75 105 0 0
  eval cursor3 SetFocalPoint 30 90 0
  cursor3 AllOff
  cursor3 AxesOn
  cursor3 OutlineOff
  cursor3 PointOn
  cursor3 SetRadius 3
vtkPolyDataMapper2D cursor3Mapper
  cursor3Mapper SetInputConnection [cursor3 GetOutputPort]
vtkActor2D cursor3Actor
  cursor3Actor SetMapper cursor3Mapper
  [cursor3Actor GetProperty] SetColor 0 1 0

vtkCursor2D cursor4
  eval cursor4 SetModelBounds 75 105 75 105 0 0
  eval cursor4 SetFocalPoint 90 90 0
  cursor4 AllOff
  cursor4 AxesOn
  cursor4 SetRadius 0.0
vtkPolyDataMapper2D cursor4Mapper
  cursor4Mapper SetInputConnection [cursor4 GetOutputPort]
vtkActor2D cursor4Actor
  cursor4Actor SetMapper cursor4Mapper
  [cursor4Actor GetProperty] SetColor 1 0 0

  # rendering support
vtkRenderer ren1
vtkRenderWindow renWin
  renWin AddRenderer ren1
vtkRenderWindowInteractor iren
  iren SetRenderWindow renWin

  # read data
ren1 AddActor cursorActor
ren1 AddActor cursor2Actor
ren1 AddActor cursor3Actor
ren1 AddActor cursor4Actor

ren1 SetBackground 0 0 0
renWin SetSize 120 120
renWin Render

iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .
