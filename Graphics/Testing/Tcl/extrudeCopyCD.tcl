package require vtktcl_interactor

vtkDiskSource disk
  disk SetRadialResolution 2
  disk SetCircumferentialResolution 9
vtkTriangleFilter tri
  tri SetInput [disk GetOutput]
  tri Update


vtkTransform t
  t Translate 1.1 0 0
vtkTransformFilter tf
  tf SetTransform t
  tf SetInput [tri GetOutput]
vtkStripper strips
  strips SetInput [tf GetOutput]

vtkAppendPolyData app
  app AddInput [tri GetOutput]
  app AddInput [strips GetOutput]
  app Update

set model [app GetOutput]

vtkLinearExtrusionFilter extrude
  extrude SetInput $model

# disconnect the triangle filter from the model so we can
# alter it.
app SetOutput {}

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



# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkPolyDataMapper mapper
   mapper SetInput [extrude GetOutput]

vtkActor actor
    actor SetMapper mapper

# Add the actors to the renderer, set the background and size
#
ren1 AddActor actor

ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 300 300

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

set cam1 [ren1 GetActiveCamera]
$cam1 Azimuth 20
$cam1 Elevation 40
ren1 ResetCamera
$cam1 Zoom 1.5

iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .


