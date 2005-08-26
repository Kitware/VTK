package require vtk
package require vtkinteraction

# Quadric definition
vtkQuadric quadric
  quadric SetCoefficients .5 1 .2 0 .1 0 0 .2 0 0

vtkSampleFunction sample
  sample SetSampleDimensions 30 30 30
  sample SetImplicitFunction quadric
  sample Update
  #sample Print
  sample ComputeNormalsOff

# Extract cells that contains isosurface of interest
vtkConnectivityFilter conn
  conn SetInputConnection [sample GetOutputPort]
  conn ScalarConnectivityOn
  conn SetScalarRange 0.6 0.6
  conn SetExtractionModeToCellSeededRegions
  conn AddSeed 105

# Create a surface 
vtkContourFilter contours
  contours SetInputConnection [conn GetOutputPort]
#  contours SetInputConnection [sample GetOutputPort]
  contours GenerateValues 5 0.0 1.2

vtkDataSetMapper contMapper
#  contMapper SetInputConnection [contours GetOutputPort]
  contMapper SetInputConnection [conn GetOutputPort]
  contMapper SetScalarRange 0.0 1.2

vtkActor contActor
  contActor SetMapper contMapper

# Create outline
vtkOutlineFilter outline
  outline SetInputConnection [sample GetOutputPort]

vtkPolyDataMapper outlineMapper
  outlineMapper SetInputConnection [outline GetOutputPort]

vtkActor outlineActor
  outlineActor SetMapper outlineMapper
  eval [outlineActor GetProperty] SetColor 0 0 0

# Graphics
# create a window to render into
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1

# create a renderer

# interactiver renderer catches mouse events (optional)
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 SetBackground 1 1 1
ren1 AddActor contActor
ren1 AddActor outlineActor
ren1 ResetCamera
[ren1 GetActiveCamera] Zoom 1.4
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

wm withdraw .
