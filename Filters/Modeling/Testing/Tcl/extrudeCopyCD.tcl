package require vtk
package require vtkinteraction

vtkVectorText disk
  disk SetText "o"

vtkTransform t
  t Translate 1.1 0 0
vtkTransformFilter tf
  tf SetTransform t
  tf SetInputConnection [disk GetOutputPort]
vtkStripper strips
  strips SetInputConnection [tf GetOutputPort]
  strips Update

vtkAppendPolyData app
  app AddInputData [disk GetOutput]
  app AddInputData [strips GetOutput]
  app Update

set model [app GetOutput]

vtkLinearExtrusionFilter extrude
  extrude SetInputData $model

# create random cell scalars for the model before extrusion.

vtkMath rn
  rn RandomSeed 1230
vtkUnsignedCharArray cellColors
  cellColors SetNumberOfComponents 3
  cellColors SetNumberOfTuples [$model GetNumberOfCells]
for { set i 0 } { $i < [$model GetNumberOfCells] } { incr i } {
    cellColors InsertComponent $i 0 [rn Random 100 255]
    cellColors InsertComponent $i 1 [rn Random 100 255]
    cellColors InsertComponent $i 2 [rn Random 100 255]
}

[$model GetCellData] SetScalars cellColors



# Lets test the arrow source instead of creating another test.
vtkArrowSource arrow1
vtkPolyDataMapper mapper1
  mapper1 SetInputConnection [arrow1 GetOutputPort]
vtkActor actor1
  actor1 SetMapper mapper1
  actor1 SetPosition 0 -0.2 1

vtkArrowSource arrow2
  arrow2 SetShaftResolution 2
  arrow2 SetTipResolution 1
vtkPolyDataMapper mapper2
  mapper2 SetInputConnection [arrow2 GetOutputPort]
vtkActor actor2
  actor2 SetMapper mapper2
  actor2 SetPosition 1 -0.2 1





# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkPolyDataMapper mapper
   mapper SetInputConnection [extrude GetOutputPort]

vtkActor actor
    actor SetMapper mapper

# Add the actors to the renderer, set the background and size
#
ren1 AddActor actor
ren1 AddActor actor1
ren1 AddActor actor2

ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 300 300

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

set cam1 [ren1 GetActiveCamera]
$cam1 Azimuth 20
$cam1 Elevation 40
ren1 ResetCamera
$cam1 Zoom 1.5

iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .


