catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

set NUMBER_OF_PIECES 4

# include get the vtk interactor ui
source $VTK_TCL/vtkInt.tcl
source $VTK_TCL/colors.tcl

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
vtkAppendPolyData append1
    # append will ask for different pieces form each of its inputs.
    append1 ParallelStreamingOn
    for {set i 0} {$i < $NUMBER_OF_PIECES} {incr i} {
      # If we did not duplicate subdivision filter, all inputs to append would be the same.
      # Because the pipline does not buffer inputs, the single input cannot have
      # different pieces ...  I am not willing at this point to buffer inputs.
      # We may consider doing it the future (filter loop with input = output).

      vtkLoopSubdivisionFilter subdivide$i
        subdivide$i SetInput [piece GetOutput]
        subdivide$i SetNumberOfSubdivisions 4

      append1 AddInput [subdivide$i GetOutput]
    }
vtkPolyDataMapper mapper1
    mapper1 SetInput [append1 GetOutput]
    mapper1 ScalarVisibilityOff
vtkActor actor1
    actor1 SetMapper mapper1
    eval [actor1 GetProperty] SetColor $english_red




# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin


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

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

set cam1 [ren1 GetActiveCamera]
$cam1 Zoom 1.4
iren Initialize
renWin SetFileName "goblet.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .


