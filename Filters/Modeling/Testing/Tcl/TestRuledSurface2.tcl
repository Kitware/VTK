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

vtkSphereSource sphere
  sphere SetPhiResolution 15
  sphere SetThetaResolution 30

vtkPlane plane
  plane SetNormal 1 0 0

vtkCutter cut
  cut SetInputConnection [sphere GetOutputPort]
  cut SetCutFunction plane
  cut GenerateCutScalarsOn

vtkStripper strip
  strip SetInputConnection [cut GetOutputPort]

vtkPoints points
    points InsertPoint 0 1 0 0

vtkCellArray lines
    lines InsertNextCell 2;#number of points
    lines InsertCellPoint 0
    lines InsertCellPoint 0

vtkPolyData tip
    tip SetPoints points
    tip SetLines lines

vtkAppendPolyData appendPD
  appendPD AddInputConnection [strip GetOutputPort]
  appendPD AddInputData tip

# extrude profile to make coverage
#
vtkRuledSurfaceFilter extrude
    extrude SetInputConnection [appendPD GetOutputPort]
    extrude SetRuledModeToPointWalk

vtkCleanPolyData clean
  clean SetInputConnection [extrude GetOutputPort]
  clean ConvertPolysToLinesOff

vtkPolyDataMapper mapper
  mapper SetInputConnection [clean GetOutputPort]
  mapper ScalarVisibilityOff

vtkActor actor
  actor SetMapper mapper
  [actor GetProperty] SetOpacity .4

ren1 AddActor actor
renWin SetSize 200 200

renWin Render
# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .

