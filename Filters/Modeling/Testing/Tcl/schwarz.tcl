package require vtk
package require vtkinteraction

# On older Macs, 10 is too low. Due to what looks like a driver bug
# spectral lighting behaves sort of weird and produces small differences
set threshold 30

vtkPoints points
  points InsertNextPoint 2 4 0
  points InsertNextPoint 2.6 2.6 0
  points InsertNextPoint 4 2 0
  points InsertNextPoint 1.4 4 1.4
  points InsertNextPoint 2 3 1
  points InsertNextPoint 3 2 1
  points InsertNextPoint 4 1.4 1.4
  points InsertNextPoint 0 4 2
  points InsertNextPoint 1 3 2
  points InsertNextPoint 2 2 2
  points InsertNextPoint 3 1 2
  points InsertNextPoint 4 0 2
  points InsertNextPoint 0 2.6 2.6
  points InsertNextPoint 1 2 3
  points InsertNextPoint 2 1 3
  points InsertNextPoint 2.6 0 2.6
  points InsertNextPoint 0 2 4
  points InsertNextPoint 1.4 1.4 4
  points InsertNextPoint 2 0 4

vtkCellArray faces
  faces InsertNextCell 3
  faces InsertCellPoint 0
  faces InsertCellPoint 3
  faces InsertCellPoint 4
  faces InsertNextCell 3
  faces InsertCellPoint 0
  faces InsertCellPoint 4
  faces InsertCellPoint 1
  faces InsertNextCell 3
  faces InsertCellPoint 1
  faces InsertCellPoint 4
  faces InsertCellPoint 5
  faces InsertNextCell 3
  faces InsertCellPoint 1
  faces InsertCellPoint 5
  faces InsertCellPoint 2
  faces InsertNextCell 3
  faces InsertCellPoint 2
  faces InsertCellPoint 5
  faces InsertCellPoint 6
  faces InsertNextCell 3
  faces InsertCellPoint 3
  faces InsertCellPoint 7
  faces InsertCellPoint 8
  faces InsertNextCell 3
  faces InsertCellPoint 3
  faces InsertCellPoint 8
  faces InsertCellPoint 4
  faces InsertNextCell 3
  faces InsertCellPoint 4
  faces InsertCellPoint 8
  faces InsertCellPoint 9
  faces InsertNextCell 3
  faces InsertCellPoint 4
  faces InsertCellPoint 9
  faces InsertCellPoint 5
  faces InsertNextCell 3
  faces InsertCellPoint 5
  faces InsertCellPoint 9
  faces InsertCellPoint 10
  faces InsertNextCell 3
  faces InsertCellPoint 5
  faces InsertCellPoint 10
  faces InsertCellPoint 6
  faces InsertNextCell 3
  faces InsertCellPoint 6
  faces InsertCellPoint 10
  faces InsertCellPoint 11
  faces InsertNextCell 3
  faces InsertCellPoint 7
  faces InsertCellPoint 12
  faces InsertCellPoint 8
  faces InsertNextCell 3
  faces InsertCellPoint 8
  faces InsertCellPoint 12
  faces InsertCellPoint 13
  faces InsertNextCell 3
  faces InsertCellPoint 8
  faces InsertCellPoint 13
  faces InsertCellPoint 9
  faces InsertNextCell 3
  faces InsertCellPoint 9
  faces InsertCellPoint 13
  faces InsertCellPoint 14
  faces InsertNextCell 3
  faces InsertCellPoint 9
  faces InsertCellPoint 14
  faces InsertCellPoint 10
  faces InsertNextCell 3
  faces InsertCellPoint 10
  faces InsertCellPoint 14
  faces InsertCellPoint 15
  faces InsertNextCell 3
  faces InsertCellPoint 10
  faces InsertCellPoint 15
  faces InsertCellPoint 11
  faces InsertNextCell 3
  faces InsertCellPoint 12
  faces InsertCellPoint 16
  faces InsertCellPoint 13
  faces InsertNextCell 3
  faces InsertCellPoint 13
  faces InsertCellPoint 16
  faces InsertCellPoint 17
  faces InsertNextCell 3
  faces InsertCellPoint 13
  faces InsertCellPoint 17
  faces InsertCellPoint 14
  faces InsertNextCell 3
  faces InsertCellPoint 14
  faces InsertCellPoint 17
  faces InsertCellPoint 18
  faces InsertNextCell 3
  faces InsertCellPoint 14
  faces InsertCellPoint 18
  faces InsertCellPoint 15

vtkPolyData model
  model SetPolys faces
  model SetPoints points

vtkMath rn

vtkUnsignedCharArray cellColors
  cellColors SetNumberOfComponents 3
  cellColors SetNumberOfTuples [model GetNumberOfCells]
for { set i 0 } { $i < [model GetNumberOfCells] } { incr i } {
    cellColors InsertComponent $i 0 [rn Random 100 255]
    cellColors InsertComponent $i 1 [rn Random 100 255]
    cellColors InsertComponent $i 2 [rn Random 100 255]
}

[model GetCellData] SetScalars cellColors

vtkTransform t0
  t0 Identity
vtkTransformPolyDataFilter tf0
  tf0 SetTransform t0
  tf0 SetInputData model

vtkTransform t1
  t1 Identity
  t1 RotateZ 90
vtkTransformPolyDataFilter tf1
  tf1 SetTransform t1
  tf1 SetInputData model

vtkTransform t2
  t2 Identity
  t2 RotateZ 180
vtkTransformPolyDataFilter tf2
  tf2 SetTransform t2
  tf2 SetInputData model

vtkTransform t3
  t3 Identity
  t3 RotateZ 270
vtkTransformPolyDataFilter tf3
  tf3 SetTransform t3
  tf3 SetInputData model

vtkAppendPolyData af
  af AddInputConnection [tf0 GetOutputPort]
  af AddInputConnection [tf1 GetOutputPort]
  af AddInputConnection [tf2 GetOutputPort]
  af AddInputConnection [tf3 GetOutputPort]

vtkTransform t4
  t4 Identity
  t4 RotateX 180
vtkTransformPolyDataFilter tf4
  tf4 SetTransform t4
  tf4 SetInputConnection [af GetOutputPort]

vtkAppendPolyData af2
  af2 AddInputConnection [af GetOutputPort]
  af2 AddInputConnection [tf4 GetOutputPort]

vtkTransform t5
  t5 Identity
  t5 Translate 0 0 -8
vtkTransformPolyDataFilter tf5
  tf5 SetTransform t5
  tf5 SetInputConnection [af2 GetOutputPort]

vtkAppendPolyData af3
  af3 AddInputConnection [af2 GetOutputPort]
  af3 AddInputConnection [tf5 GetOutputPort]

vtkTransform t6
  t6 Identity
  t6 Translate 0 -8 0
vtkTransformPolyDataFilter tf6
  tf6 SetTransform t6
  tf6 SetInputConnection [af3 GetOutputPort]

vtkAppendPolyData af4
  af4 AddInputConnection [af3 GetOutputPort]
  af4 AddInputConnection [tf6 GetOutputPort]

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkCleanPolyData clean
  clean SetTolerance .001
  clean SetInputData model
  clean SetInputConnection [af2 GetOutputPort]
  clean SetInputConnection [af3 GetOutputPort]
  clean SetInputConnection [af4 GetOutputPort]

vtkButterflySubdivisionFilter subdivide
  subdivide SetInputConnection [clean GetOutputPort]
  subdivide SetNumberOfSubdivisions 3

vtkDataSetMapper mapper
   mapper SetInputConnection [subdivide GetOutputPort]

vtkActor surface
    surface SetMapper mapper

vtkFeatureEdges fe
  fe SetInputConnection [subdivide GetOutputPort]
  fe SetFeatureAngle 100

vtkStripper feStripper
  feStripper SetInputConnection [fe GetOutputPort]

vtkTubeFilter feTubes
  feTubes SetInputConnection [feStripper GetOutputPort]
  feTubes SetRadius .1

vtkPolyDataMapper feMapper
  feMapper SetInputConnection [feTubes GetOutputPort]

vtkActor edges
  edges SetMapper feMapper

# Add the actors to the renderer, set the background and size
#
ren1 AddActor surface
ren1 AddActor edges
vtkProperty backP
  backP SetDiffuseColor 1 1 .3
surface SetBackfaceProperty backP

[edges GetProperty] SetDiffuseColor .2 .2 .2
[surface GetProperty] SetDiffuseColor 1 .4 .3
[surface GetProperty] SetSpecular .4
[surface GetProperty] SetDiffuse .8
[surface GetProperty] SetSpecularPower 40

ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 300 300

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

ren1 ResetCamera
set cam1 [ren1 GetActiveCamera]
$cam1 Azimuth 90
ren1 ResetCamera
$cam1 Zoom 1.5
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .


