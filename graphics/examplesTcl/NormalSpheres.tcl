catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }


set PIECE1 0
set PIECE2 1
set NUMBER_OF_PIECES 2

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

# create pipeline

vtkSphereSource sphere1
    sphere1 SetRadius 1
    sphere1 SetPhiResolution 8
    sphere1 SetThetaResolution 12
    sphere1 SetCenter -1.5 0 0
vtkPolyDataNormals pdn1
    pdn1 SetInput [sphere1 GetOutput]
vtkPolyDataMapper halfSphere1Mapper
    halfSphere1Mapper SetInput [pdn1 GetOutput]
    halfSphere1Mapper ScalarVisibilityOff
    halfSphere1Mapper SetPiece $PIECE1
    halfSphere1Mapper SetNumberOfPieces $NUMBER_OF_PIECES
vtkActor halfSphere1Actor
    halfSphere1Actor SetMapper halfSphere1Mapper
    eval [halfSphere1Actor GetProperty] SetColor $english_red

vtkSphereSource sphere2
    sphere2 SetRadius 1
    sphere2 SetPhiResolution 8
    sphere2 SetThetaResolution 12
    sphere2 SetCenter -1.5 0 0
vtkPolyDataNormals pdn2
    pdn2 SetInput [sphere2 GetOutput]
vtkPolyDataMapper halfSphere2Mapper
    halfSphere2Mapper SetInput [pdn2 GetOutput]
    halfSphere2Mapper ScalarVisibilityOff
    halfSphere2Mapper SetPiece $PIECE2
    halfSphere2Mapper SetNumberOfPieces $NUMBER_OF_PIECES
vtkActor halfSphere2Actor
    halfSphere2Actor SetMapper halfSphere2Mapper
    eval [halfSphere2Actor GetProperty] SetColor $english_red

vtkSphereSource halfSphere3
    halfSphere3 SetRadius 1
    halfSphere3 SetPhiResolution 8
    halfSphere3 SetThetaResolution 6
    halfSphere3 SetCenter 1.5 0 0
    halfSphere3 SetStartTheta 0.0
    halfSphere3 SetEndTheta 180.0
vtkPolyDataNormals pdn3
    pdn3 SetInput [halfSphere3 GetOutput]
vtkPolyDataMapper halfSphere3Mapper
    halfSphere3Mapper SetInput [pdn3 GetOutput]
    halfSphere3Mapper ScalarVisibilityOff
vtkActor halfSphere3Actor
    halfSphere3Actor SetMapper halfSphere3Mapper
    eval [halfSphere3Actor GetProperty] SetColor $english_red

vtkSphereSource halfSphere4
    halfSphere4 SetRadius 1
    halfSphere4 SetPhiResolution 8
    halfSphere4 SetThetaResolution 6
    halfSphere4 SetCenter 1.5 0 0
    halfSphere4 SetStartTheta 180.0
    halfSphere4 SetEndTheta 360.0
vtkPolyDataNormals pdn4
    pdn4 SetInput [halfSphere4 GetOutput]
vtkPolyDataMapper halfSphere4Mapper
    halfSphere4Mapper SetInput [pdn4 GetOutput]
    halfSphere4Mapper ScalarVisibilityOff
vtkActor halfSphere4Actor
    halfSphere4Actor SetMapper halfSphere4Mapper
    eval [halfSphere4Actor GetProperty] SetColor $english_red


# Add the actors to the renderer, set the background and size
#

[ren1 GetActiveCamera] SetPosition 5 5 10
[ren1 GetActiveCamera] SetFocalPoint 0 0 0
ren1 AddActor halfSphere1Actor
ren1 AddActor halfSphere2Actor
ren1 AddActor halfSphere3Actor
ren1 AddActor halfSphere4Actor
ren1 SetBackground 1 1 1
renWin SetSize 500 500
iren Initialize

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .


