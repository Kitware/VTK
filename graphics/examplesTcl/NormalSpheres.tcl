catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }


set NUMBER_OF_PIECES 7

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

vtkSphereSource sphere1
    sphere1 SetRadius 1
    sphere1 SetPhiResolution 9
    sphere1 SetThetaResolution 12
    sphere1 SetCenter -1.5 0 0
vtkExtractPolyDataPiece piece1
    piece1 SetInput [sphere1 GetOutput]
vtkAppendPolyData append1
    # append will ask for different pieces form each of its inputs.
    append1 ParallelStreamingOn
    for {set i 0} {$i < $NUMBER_OF_PIECES} {incr i} {
      # If we did not duplicate pdn filter, all inputs to append would be the same.
      # Because the pipline does not buffer inputs, the single input cannot have
      # different pieces ...  I am not willing at this point to buffer inputs.
      # We may consider doing it the future (filter loop with input = output).
       vtkPolyDataNormals pdn1$i
         pdn1$i SetInput [piece1 GetOutput]
      append1 AddInput [pdn1$i GetOutput]
    }
vtkPolyDataMapper mapper1
    mapper1 SetInput [append1 GetOutput]
    mapper1 ScalarVisibilityOff
vtkActor actor1
    actor1 SetMapper mapper1
    eval [actor1 GetProperty] SetColor $english_red


# create the second pipeline, but disable ghost cells. 

vtkSphereSource sphere2
    sphere2 SetRadius 1
    sphere2 SetPhiResolution 9
    sphere2 SetThetaResolution 12
    sphere2 SetCenter 1.5 0 0
vtkExtractPolyDataPiece piece2
    piece2 SetInput [sphere2 GetOutput]
    piece2 CreateGhostCellsOff
vtkAppendPolyData append2
    # append will ask for different pieces form each of its inputs.
    append2 ParallelStreamingOn
    for {set i 0} {$i < $NUMBER_OF_PIECES} {incr i} {
      # If we did not duplicate pdn filter, all inputs to append would be the same.
      # Because the pipline does not buffer inputs, the single input cannot have
      # different pieces ...  I am not willing at this point to buffer inputs.
      # We may consider doing it the future (filter loop with input = output).
       vtkPolyDataNormals pdn2$i
         pdn2$i SetInput [piece2 GetOutput]
      append2 AddInput [pdn2$i GetOutput]
    }
vtkPolyDataMapper mapper2
    mapper2 SetInput [append2 GetOutput]
    mapper2 ScalarVisibilityOff
vtkActor actor2
    actor2 SetMapper mapper2
    eval [actor2 GetProperty] SetColor $english_red



# Add the actors to the renderer, set the background and size
#

[ren1 GetActiveCamera] SetPosition 5 5 10
[ren1 GetActiveCamera] SetFocalPoint 0 0 0
ren1 AddActor actor1
ren1 AddActor actor2
ren1 SetBackground 1 1 1
renWin SetSize 500 500
iren Initialize

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .


