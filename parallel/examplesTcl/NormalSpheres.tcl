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
vtkPPolyDataNormals norms1
    norms1 SetInput [piece1 GetOutput]
vtkPolyDataStreamer stream1
    stream1 SetInput [piece1 GetOutput]
    stream1 SetNumberOfStreamDivisions $NUMBER_OF_PIECES 
vtkPolyDataMapper mapper1
    mapper1 SetInput [stream1 GetOutput]
    mapper1 ScalarVisibilityOff
vtkActor actor1
    actor1 SetMapper mapper1
    eval [actor1 GetProperty] SetColor 0.9 0.8 0.5


# create the second pipeline, but disable ghost cells. 

vtkSphereSource sphere2
    sphere2 SetRadius 1
    sphere2 SetPhiResolution 9
    sphere2 SetThetaResolution 12
    sphere2 SetCenter 1.5 0 0
vtkExtractPolyDataPiece piece2
    piece2 SetInput [sphere2 GetOutput]
    piece2 CreateGhostCellsOff
vtkPolyDataNormals norms2
    norms2 SetInput [piece2 GetOutput]
vtkPolyDataStreamer stream2
    stream2 SetInput [norms2 GetOutput]
    stream2 SetNumberOfStreamDivisions $NUMBER_OF_PIECES 
vtkPolyDataMapper mapper2
    mapper2 SetInput [stream2 GetOutput]
    mapper2 ScalarVisibilityOff
vtkActor actor2
    actor2 SetMapper mapper2
    eval [actor2 GetProperty] SetColor 0.9 0.8 0.5



# Add the actors to the renderer, set the background and size
#

[ren1 GetActiveCamera] SetPosition 5 5 10
[ren1 GetActiveCamera] SetFocalPoint 0 0 0
ren1 AddActor actor1
ren1 AddActor actor2
ren1 SetBackground 0 0 0.8
renWin SetSize 500 500
iren Initialize

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .


