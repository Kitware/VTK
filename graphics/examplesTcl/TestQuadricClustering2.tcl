catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }


# This test whether the match boundaries feature works.
# I plan to add a simple UI to control this script.

set NUMBER_OF_PIECES 5

# Generate implicit model of a sphere
#
source $VTK_TCL/vtkInt.tcl
source $VTK_TCL/colors.tcl

# Create renderer stuff
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create pipeline that handles ghost cells


vtkBYUReader byu
    byu SetGeometryFileName "$VTK_DATA/fohe.g"

vtkExtractPolyDataPiece piece
  piece SetInput [byu GetOutput]

vtkSphereSource sphere
    sphere SetRadius 3
    sphere SetPhiResolution 100
    sphere SetThetaResolution 150
    #sphere SetPhiResolution 3
    #sphere SetThetaResolution 4
    # sphere SetStartMethod {tk_messageBox -message "Executing with piece [[sphere GetOutput] GetUpdatePiece]"}

# Since quadric Clustering does not handle borders properly yet,
# the pieces will have dramatic "eams"
vtkQuadricClustering q
  q SetInput [sphere GetOutput]
  #q SetInput [piece GetOutput]
  q SetDivisionOrigin 0 0 0
  q SetDivisionSpacing 0.5 0.5 0.5
  q SetDivisionOrigin 0.1 0.1 0.1
  #q UseInputPointsOn
  q UseFeatureEdgesOn

vtkPolyDataStreamer streamer
  streamer SetInput [q GetOutput]
  streamer SetNumberOfStreamDivisions $NUMBER_OF_PIECES
  streamer ColorByPieceOn

vtkPolyDataMapper mapper
  mapper SetInput [streamer GetOutput]
  #mapper SetInput [q GetOutput]
  #mapper ScalarVisibilityOff
  mapper SetScalarRange 0 $NUMBER_OF_PIECES
  mapper SetPiece 0
  mapper SetNumberOfPieces 2
  mapper ImmediateModeRenderingOn


vtkProperty backProperty
  backProperty SetColor 1 1 0

vtkActor actor
    actor SetMapper mapper
    #eval [actor GetProperty] SetColor $english_red
    #actor SetBackfaceProperty backProperty


# Add the actors to the renderer, set the background and size
#

[ren1 GetActiveCamera] SetPosition 5 5 10
[ren1 GetActiveCamera] SetFocalPoint 0 0 0
ren1 AddActor actor
ren1 SetBackground 1 1 1
renWin SetSize 500 500
iren Initialize

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .


