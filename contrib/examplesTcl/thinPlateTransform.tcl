# Test the vtkThinPlateSplineTransform class
#

catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# this is a tcl version of the Mace example
# get the interactor ui

source $VTK_TCL/vtkInt.tcl
source $VTK_TCL/colors.tcl

# create a sphere source and actor
#
vtkSphereSource original
    original SetThetaResolution 100
    original SetPhiResolution 100

vtkPolyDataNormals normals
    normals SetInput [original GetOutput]

vtkPoints spoints
    spoints SetNumberOfPoints 6
vtkPoints tpoints
    tpoints SetNumberOfPoints 6

spoints SetPoint 0 0 0 0
tpoints SetPoint 0 0 0 0
spoints SetPoint 1 1 0 0
tpoints SetPoint 1 1 0 0
spoints SetPoint 2 0 1 0
tpoints SetPoint 2 0 1 0
spoints SetPoint 3 1 1 1
tpoints SetPoint 3 1 1 0.5
spoints SetPoint 4 -1 1 2
tpoints SetPoint 4 -1 1 3
spoints SetPoint 5 0.5 0.5 2
tpoints SetPoint 5 0.5 0.5 1

vtkThinPlateSplineTransform trans
    trans SetSourceLandmarks spoints
    trans SetTargetLandmarks tpoints
    trans SetBasisToR
# yeah, this is silly -- improves code coverage though
vtkGeneralTransformConcatenation transconcat
    transconcat Concatenate trans
    transconcat Concatenate [trans GetInverse]
    transconcat Concatenate trans

vtkTransformPolyDataFilter warp
    warp SetInput [original GetOutput]
    warp SetTransform transconcat

vtkPolyDataMapper mapper
    mapper SetInput [warp GetOutput]

vtkProperty backProp
    eval backProp SetDiffuseColor $tomato
vtkActor actor
    actor SetMapper mapper
    eval [actor GetProperty] SetColor $peacock
    actor SetBackfaceProperty backProp

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
    renWin SetSize 100 250
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor actor
ren1 SetBackground 1 1 1
renWin SetSize 200 300

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

set cam1 [ren1 GetActiveCamera]
$cam1 SetClippingRange 1.87 3.9
$cam1 SetFocalPoint 0 0 0.254605
$cam1 SetPosition 0.571764 2.8232 0.537528
$cam1 SetViewUp 0.5188 -0.0194195 -0.854674
renWin Render

renWin SetFileName "thinPlateTransform.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .


