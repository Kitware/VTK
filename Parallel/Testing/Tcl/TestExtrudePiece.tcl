package require vtk
package require vtkinteraction


vtkDiskSource disk
  disk SetRadialResolution 2
  disk SetCircumferentialResolution 9

vtkCleanPolyData clean
  clean SetInputConnection [disk GetOutputPort]
  clean SetTolerance 0.01

vtkExtractPolyDataPiece piece
  piece SetInputConnection [clean GetOutputPort]

vtkPLinearExtrusionFilter extrude
  extrude SetInputConnection [piece GetOutputPort]
  extrude PieceInvariantOn



# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkPolyDataMapper mapper
   mapper SetInputConnection [extrude GetOutputPort]
   mapper SetNumberOfPieces 2
   mapper SetPiece 1

vtkProperty bf
    bf SetColor 1 0 0
vtkActor actor
    actor SetMapper mapper
    [actor GetProperty] SetColor 1 1 0.8
    actor SetBackfaceProperty bf



# Add the actors to the renderer, set the background and size
#
ren1 AddActor actor

ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 300 300

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

set cam1 [ren1 GetActiveCamera]
$cam1 Azimuth 20
$cam1 Elevation 40
ren1 ResetCamera
$cam1 Zoom 1.2

iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .


