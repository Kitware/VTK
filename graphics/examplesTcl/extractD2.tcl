catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# extract data plus boundary
# get the interactor ui
source $VTK_TCL/vtkInt.tcl
source $VTK_TCL/colors.tcl

vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkQuadric quadric
    quadric SetCoefficients .5 1 .2 0 .1 0 0 .2 0 0
vtkSampleFunction sample
    sample SetSampleDimensions 50 50 50
    sample SetImplicitFunction quadric
    sample ComputeNormalsOff
vtkTransform trans
    trans Scale 1 .5 .333
vtkSphere sphere
    sphere SetRadius 0.25
    sphere SetTransform trans
vtkTransform trans2
    trans2 Scale .25 .5 1.0
vtkSphere sphere2
    sphere2 SetRadius 0.25
    sphere2 SetTransform trans2
vtkImplicitBoolean union
    union AddFunction sphere
    union AddFunction sphere2
    union SetOperationType 0;#union
vtkExtractGeometry extract
    extract SetInput [sample GetOutput]
    extract SetImplicitFunction union
    extract ExtractBoundaryCellsOn
vtkShrinkFilter shrink
    shrink SetInput [extract GetOutput]
    shrink SetShrinkFactor 0.5
vtkDataSetMapper dataMapper
    dataMapper SetInput [shrink GetOutput]
vtkActor dataActor
    dataActor SetMapper dataMapper

# outline
vtkOutlineFilter outline
    outline SetInput [sample GetOutput]
vtkPolyDataMapper outlineMapper
    outlineMapper SetInput [outline GetOutput]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper
set outlineProp [outlineActor GetProperty]
eval $outlineProp SetColor 0 0 0

# Add the actors to the renderer, set the background and size
#
ren1 AddActor outlineActor
ren1 AddActor dataActor
ren1 SetBackground 1 1 1
renWin SetSize 500 500
[ren1 GetActiveCamera] Zoom 1.5
iren Initialize

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

#renWin SetFileName "valid/extractD.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .
