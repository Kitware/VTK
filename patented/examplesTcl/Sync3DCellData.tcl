catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# Generate marching cubes head model (full resolution)

# get the interactor ui and colors
source $VTK_TCL/vtkInt.tcl
source $VTK_TCL/colors.tcl

vtkImageData d
d SetDimensions 3 3 3
d SetScalarType 10
d AllocateScalars
set s [[d GetPointData] GetScalars]
$s SetScalar 0 0
$s SetScalar 1 0
$s SetScalar 2 0
$s SetScalar 3 0
$s SetScalar 4 1
$s SetScalar 5 0
$s SetScalar 6 0
$s SetScalar 7 0
$s SetScalar 8 0

$s SetScalar 9 0
$s SetScalar 10 1
$s SetScalar 11 0
$s SetScalar 12 1
$s SetScalar 13 0
$s SetScalar 14 1
$s SetScalar 15 0
$s SetScalar 16 1
$s SetScalar 17 0

$s SetScalar 18 0
$s SetScalar 19 0
$s SetScalar 20 0
$s SetScalar 21 0
$s SetScalar 22 1
$s SetScalar 23 0
$s SetScalar 24 0
$s SetScalar 25 0
$s SetScalar 26 0

vtkScalars cs
cs SetNumberOfScalars 8
cs SetScalar 0 0
cs SetScalar 1 1
cs SetScalar 2 2
cs SetScalar 3 3
cs SetScalar 4 4
cs SetScalar 5 5
cs SetScalar 6 6
cs SetScalar 7 7

[d GetCellData] SetScalars cs








# write isosurface to file
vtkSynchronizedTemplates3D stemp
#vtkKitwareContourFilter stemp
    stemp SetInput d
    stemp SetValue 0 0.5
    stemp ComputeScalarsOff

vtkPolyDataMapper mapper
    mapper SetInput [stemp GetOutput]
    mapper ScalarVisibilityOn
    mapper SetScalarRange 0 7

vtkActor head
    head SetMapper mapper

# Create the RenderWindow, Renderer and Interactor
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor head
ren1 SetBackground 1 1 1
renWin SetSize 500 500
#eval ren1 SetBackground $slate_grey

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}


set cam [ren1 GetActiveCamera]
$cam SetFocalPoint 1 1 1
$cam SetPosition 0 0 0
ren1 ResetCamera
$cam Zoom 1.3
renWin Render

#renWin SetFileName "genHead.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .

