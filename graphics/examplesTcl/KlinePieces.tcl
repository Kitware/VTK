catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# Source the interactor that we will use for the TkRenderWidget
source $VTK_TCL/../graphics/examplesTcl/TkInteractor.tcl

# user interface command widget
source $VTK_TCL/vtkInt.tcl

vtkOutputWindow w
w PromptUserOn


# ------------------- Create the UI ---------------------

# UI Variables
set NUMBER_OF_PIECES 4
set NUMBER_OF_SUBDIVISIONS 4
set GHOST_FLAG 0



# prevent the tk window from showing up then start the event loop
wm withdraw .

# Create the toplevel window
toplevel .top
wm title .top {Unstructured/Polydata Piece/GhostCell Demostration}

# Create some frames
frame .top.f1 
frame .top.f2
pack .top.f1 .top.f2 -side top -expand 1 -fill both

vtkRenderWindow renWin
vtkTkRenderWidget .top.f1.rw -width 400 -height 400 -rw renWin
BindTkRenderWidget .top.f1.rw
pack .top.f1.rw -expand 1 -fill both

# create a rendering window and renderer
vtkRenderer ren1
    renWin AddRenderer ren1

scale .top.f2.s1 -label " Number Of Pieces: " -orient horizontal \
	-length 200 -from 1 -to 32 -variable NUMBER_OF_PIECES 
scale .top.f2.s2 -label " Number Of Subdivision: " -orient horizontal \
	-length 200 -from 0 -to 5 -variable NUMBER_OF_SUBDIVISIONS 

checkbutton .top.f2.check -text "GhostCellsActive" \
	-command "ToggleGhostCells" -variable GHOST_FLAG

pack .top.f2.s1 .top.f2.s2 .top.f2.check -side top -expand 1 -fill both

button .top.f2.b1 -text "Quit" -command {vtkCommand DeleteAllObjects; exit}
pack .top.f2.b1  -expand 1 -fill x

bind .top.f2.s1 <ButtonRelease> { 
   streamer SetNumberOfStreamDivisions $NUMBER_OF_PIECES
   mapper1 SetScalarRange 0 $NUMBER_OF_PIECES
   #mapper1 SetNumberOfPieces $NUMBER_OF_PIECES
   renWin Render
}

bind .top.f2.s2 <ButtonRelease> { 
   subdivide SetNumberOfSubdivisions $NUMBER_OF_SUBDIVISIONS
   renWin Render
}

proc ToggleGhostCells {} {
	global GHOST_FLAG
	piece SetCreateGhostCells $GHOST_FLAG
	renWin Render
}


#--------------------------------



vtkPoints points
points InsertNextPoint 0 -16 0 
points InsertNextPoint 0 0 -14 
points InsertNextPoint 0 0 14 
points InsertNextPoint 14 0 0 
points InsertNextPoint 10 20 -10 
points InsertNextPoint 10 20 10 
points InsertNextPoint 10 -20 -10 
points InsertNextPoint 10 -20 10 
points InsertNextPoint -10 -20 -10 
points InsertNextPoint -10 -20 10 
points InsertNextPoint -10 20 -10 
points InsertNextPoint -10 20 10 
points InsertNextPoint -2 27 0 
points InsertNextPoint 0 27 2 
points InsertNextPoint 0 27 -2 
points InsertNextPoint 2 27 0 
points InsertNextPoint -14 4 -1 
points InsertNextPoint -14 3 0 
points InsertNextPoint -14 5 0 
points InsertNextPoint -14 4 1 
points InsertNextPoint -1 38 -2 
points InsertNextPoint -1 38 2 
points InsertNextPoint 2 35 -2 
points InsertNextPoint 2 35 2 
points InsertNextPoint 17 42 0 
points InsertNextPoint 15 40 2 
points InsertNextPoint 15 39 -2 
points InsertNextPoint 13 37 0 
points InsertNextPoint 19 -2 -2 
points InsertNextPoint 19 -2 2 
points InsertNextPoint 15 2 -2 
points InsertNextPoint 15 2 2 

vtkCellArray faces
faces InsertNextCell 3
faces InsertCellPoint 3
faces InsertCellPoint 4
faces InsertCellPoint 5
faces InsertNextCell 3
faces InsertCellPoint 3
faces InsertCellPoint 5
faces InsertCellPoint 7
faces InsertNextCell 3
faces InsertCellPoint 3
faces InsertCellPoint 7
faces InsertCellPoint 6
faces InsertNextCell 3
faces InsertCellPoint 3
faces InsertCellPoint 6
faces InsertCellPoint 4
faces InsertNextCell 3
faces InsertCellPoint 0
faces InsertCellPoint 6
faces InsertCellPoint 7
faces InsertNextCell 3
faces InsertCellPoint 0
faces InsertCellPoint 7
faces InsertCellPoint 9
faces InsertNextCell 3
faces InsertCellPoint 0
faces InsertCellPoint 9
faces InsertCellPoint 8
faces InsertNextCell 3
faces InsertCellPoint 0
faces InsertCellPoint 8
faces InsertCellPoint 6
faces InsertNextCell 3
faces InsertCellPoint 1
faces InsertCellPoint 4
faces InsertCellPoint 6
faces InsertNextCell 3
faces InsertCellPoint 1
faces InsertCellPoint 6
faces InsertCellPoint 8
faces InsertNextCell 3
faces InsertCellPoint 1
faces InsertCellPoint 8
faces InsertCellPoint 10
faces InsertNextCell 3
faces InsertCellPoint 1
faces InsertCellPoint 10
faces InsertCellPoint 4
faces InsertNextCell 3
faces InsertCellPoint 2
faces InsertCellPoint 11
faces InsertCellPoint 9
faces InsertNextCell 3
faces InsertCellPoint 2
faces InsertCellPoint 9
faces InsertCellPoint 7
faces InsertNextCell 3
faces InsertCellPoint 2
faces InsertCellPoint 7
faces InsertCellPoint 5
faces InsertNextCell 3
faces InsertCellPoint 2
faces InsertCellPoint 5
faces InsertCellPoint 11
faces InsertNextCell 3
faces InsertCellPoint 4
faces InsertCellPoint 15
faces InsertCellPoint 5
faces InsertNextCell 3
faces InsertCellPoint 4
faces InsertCellPoint 14
faces InsertCellPoint 15
faces InsertNextCell 3
faces InsertCellPoint 5
faces InsertCellPoint 13
faces InsertCellPoint 11
faces InsertNextCell 3
faces InsertCellPoint 5
faces InsertCellPoint 15
faces InsertCellPoint 13
faces InsertNextCell 3
faces InsertCellPoint 11
faces InsertCellPoint 12
faces InsertCellPoint 10
faces InsertNextCell 3
faces InsertCellPoint 11
faces InsertCellPoint 13
faces InsertCellPoint 12
faces InsertNextCell 3
faces InsertCellPoint 10
faces InsertCellPoint 14
faces InsertCellPoint 4
faces InsertNextCell 3
faces InsertCellPoint 10
faces InsertCellPoint 12
faces InsertCellPoint 14
faces InsertNextCell 3
faces InsertCellPoint 8
faces InsertCellPoint 17
faces InsertCellPoint 16
faces InsertNextCell 3
faces InsertCellPoint 8
faces InsertCellPoint 9
faces InsertCellPoint 17
faces InsertNextCell 3
faces InsertCellPoint 9
faces InsertCellPoint 19
faces InsertCellPoint 17
faces InsertNextCell 3
faces InsertCellPoint 9
faces InsertCellPoint 11
faces InsertCellPoint 19
faces InsertNextCell 3
faces InsertCellPoint 11
faces InsertCellPoint 18
faces InsertCellPoint 19
faces InsertNextCell 3
faces InsertCellPoint 11
faces InsertCellPoint 10
faces InsertCellPoint 18
faces InsertNextCell 3
faces InsertCellPoint 10
faces InsertCellPoint 16
faces InsertCellPoint 18
faces InsertNextCell 3
faces InsertCellPoint 10
faces InsertCellPoint 8
faces InsertCellPoint 16
faces InsertNextCell 3
faces InsertCellPoint 13
faces InsertCellPoint 21
faces InsertCellPoint 12
faces InsertNextCell 3
faces InsertCellPoint 12
faces InsertCellPoint 21
faces InsertCellPoint 20
faces InsertNextCell 3
faces InsertCellPoint 12
faces InsertCellPoint 20
faces InsertCellPoint 14
faces InsertNextCell 3
faces InsertCellPoint 14
faces InsertCellPoint 20
faces InsertCellPoint 22
faces InsertNextCell 3
faces InsertCellPoint 14
faces InsertCellPoint 22
faces InsertCellPoint 15
faces InsertNextCell 3
faces InsertCellPoint 15
faces InsertCellPoint 22
faces InsertCellPoint 23
faces InsertNextCell 3
faces InsertCellPoint 15
faces InsertCellPoint 23
faces InsertCellPoint 13
faces InsertNextCell 3
faces InsertCellPoint 13
faces InsertCellPoint 23
faces InsertCellPoint 21
faces InsertNextCell 3
faces InsertCellPoint 21
faces InsertCellPoint 25
faces InsertCellPoint 24
faces InsertNextCell 3
faces InsertCellPoint 21
faces InsertCellPoint 24
faces InsertCellPoint 20
faces InsertNextCell 3
faces InsertCellPoint 20
faces InsertCellPoint 24
faces InsertCellPoint 26
faces InsertNextCell 3
faces InsertCellPoint 20
faces InsertCellPoint 26
faces InsertCellPoint 22
faces InsertNextCell 3
faces InsertCellPoint 22
faces InsertCellPoint 26
faces InsertCellPoint 27
faces InsertNextCell 3
faces InsertCellPoint 22
faces InsertCellPoint 27
faces InsertCellPoint 23
faces InsertNextCell 3
faces InsertCellPoint 23
faces InsertCellPoint 27
faces InsertCellPoint 25
faces InsertNextCell 3
faces InsertCellPoint 23
faces InsertCellPoint 25
faces InsertCellPoint 21
faces InsertNextCell 3
faces InsertCellPoint 25
faces InsertCellPoint 29
faces InsertCellPoint 24
faces InsertNextCell 3
faces InsertCellPoint 24
faces InsertCellPoint 29
faces InsertCellPoint 28
faces InsertNextCell 3
faces InsertCellPoint 24
faces InsertCellPoint 28
faces InsertCellPoint 26
faces InsertNextCell 3
faces InsertCellPoint 26
faces InsertCellPoint 28
faces InsertCellPoint 30
faces InsertNextCell 3
faces InsertCellPoint 26
faces InsertCellPoint 30
faces InsertCellPoint 27
faces InsertNextCell 3
faces InsertCellPoint 27
faces InsertCellPoint 30
faces InsertCellPoint 31
faces InsertNextCell 3
faces InsertCellPoint 27
faces InsertCellPoint 31
faces InsertCellPoint 25
faces InsertNextCell 3
faces InsertCellPoint 25
faces InsertCellPoint 31
faces InsertCellPoint 29
faces InsertNextCell 3
faces InsertCellPoint 29
faces InsertCellPoint 19
faces InsertCellPoint 17
faces InsertNextCell 3
faces InsertCellPoint 29
faces InsertCellPoint 17
faces InsertCellPoint 28
faces InsertNextCell 3
faces InsertCellPoint 28
faces InsertCellPoint 17
faces InsertCellPoint 16
faces InsertNextCell 3
faces InsertCellPoint 28
faces InsertCellPoint 16
faces InsertCellPoint 30
faces InsertNextCell 3
faces InsertCellPoint 30
faces InsertCellPoint 16
faces InsertCellPoint 18
faces InsertNextCell 3
faces InsertCellPoint 30
faces InsertCellPoint 18
faces InsertCellPoint 31
faces InsertNextCell 3
faces InsertCellPoint 31
faces InsertCellPoint 18
faces InsertCellPoint 19
faces InsertNextCell 3
faces InsertCellPoint 31
faces InsertCellPoint 19
faces InsertCellPoint 29


vtkPolyData model
  model SetPolys faces
  model SetPoints points



vtkExtractPolyDataPiece piece
  piece SetInput model
  piece SetCreateGhostCells $GHOST_FLAG

vtkLoopSubdivisionFilter subdivide
  subdivide SetInput [piece GetOutput]
  subdivide SetNumberOfSubdivisions $NUMBER_OF_SUBDIVISIONS

vtkPolyDataStreamer streamer
  streamer SetNumberOfStreamDivisions $NUMBER_OF_PIECES
  streamer SetInput [subdivide GetOutput]
  streamer ColorByPieceOn

vtkPolyDataMapper mapper1
    mapper1 SetInput [streamer GetOutput]
    #mapper1 SetInput [subdivide GetOutput]
	mapper1 SetScalarRange 0 $NUMBER_OF_PIECES

vtkActor actor1
    actor1 SetMapper mapper1




# Create the RenderWindow, Renderer and both Actors
#


# Add the actors to the renderer, set the background and size
#
ren1 AddActor actor1
vtkProperty backP
  backP SetDiffuseColor 1 1 .3
actor1 SetBackfaceProperty backP

[actor1 GetProperty] SetDiffuseColor 1 .4 .3
[actor1 GetProperty] SetSpecular .4
[actor1 GetProperty] SetDiffuse .8
[actor1 GetProperty] SetSpecularPower 40

ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 300 300


set cam1 [ren1 GetActiveCamera]
$cam1 Zoom 1.4
renWin SetFileName "goblet.tcl.ppm"
#renWin SaveImageAsPPM



