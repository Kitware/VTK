catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# get the interactor ui
source $VTK_TCL/vtkInt.tcl
source $VTK_TCL/colors.tcl
# create planes

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create pipeline
#
vtkPLOT3DReader pl3d
    pl3d SetXYZFileName "$VTK_DATA/combxyz.bin"
    pl3d SetQFileName "$VTK_DATA/combq.bin"
    pl3d SetScalarFunctionNumber 100
    pl3d SetVectorFunctionNumber 202
    pl3d Update
vtkStructuredGridGeometryFilter plane
    plane SetInput [pl3d GetOutput]
    plane SetExtent 1 100 1 100 7 7
vtkLookupTable lut
vtkPolyDataMapper planeMapper
    planeMapper SetLookupTable lut
    planeMapper SetInput [plane GetOutput]
    planeMapper SetScalarRange 0.197813 0.710419
vtkActor planeActor
    planeActor SetMapper planeMapper

vtkStructuredGridOutlineFilter outline
    outline SetInput [pl3d GetOutput]
vtkPolyDataMapper outlineMapper
    outlineMapper SetInput [outline GetOutput]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper
set outlineProp [outlineActor GetProperty]
#eval $outlineProp SetColor $black

# different lookup tables for each figure
#

#black to white lut
#    lut SetHueRange 0 0
#    lut SetSaturationRange 0 0
#    lut SetValueRange 0.2 1.0

#red to blue lut
#    lut SetHueRange 0.0 0.667

#blue to red lut
#    lut SetHueRange 0.667 0.0

#funky constrast
    lut SetNumberOfColors 256
    lut Build
    for {set i 0} {$i<16} {incr i 1} {
        eval lut SetTableValue [expr $i*16] $red 1
        eval lut SetTableValue [expr $i*16+1] $green 1
        eval lut SetTableValue [expr $i*16+2] $blue 1
        eval lut SetTableValue [expr $i*16+3] $black 1
    }
#    eval lut SetTableValue 0 $coral 1
#    eval lut SetTableValue 1 $black 1
#    eval lut SetTableValue 2 $peacock 1
#    eval lut SetTableValue 3 $black 1
#    eval lut SetTableValue 4 $orchid 1
#    eval lut SetTableValue 5 $black 1
#    eval lut SetTableValue 6 $cyan 1
#    eval lut SetTableValue 7 $black 1
#    eval lut SetTableValue 8 $mint 1
#    eval lut SetTableValue 9 $black 1
#    eval lut SetTableValue 10 $tomato 1
#    eval lut SetTableValue 11 $black 1
#    eval lut SetTableValue 12 $sea_green 1
#    eval lut SetTableValue 13 $black 1
#    eval lut SetTableValue 14 $plum 1
#    eval lut SetTableValue 15 $black 1

# Add the actors to the renderer, set the background and size
#
ren1 AddActor outlineActor
ren1 AddActor planeActor
#ren1 SetBackground 1 1 1
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 500 500
renWin DoubleBufferOn
iren Initialize

renWin Render

set cam1 [ren1 GetActiveCamera]
$cam1 SetClippingRange 3.95297 50
$cam1 SetFocalPoint 8.88908 0.595038 29.3342
$cam1 SetPosition -12.3332 31.7479 41.2387
$cam1 SetViewUp 0.060772 -0.319905 0.945498

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

renWin Render

#renWin SetFileName "rainbow.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .



