catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }


# This script test the abbility for SynchronizedTemplates2D to handle images on every axis,
# and images larger than requested.


# get the interactor ui
source $VTK_TCL/vtkInt.tcl
source $VTK_TCL/colors.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create a simple data set (3x3x3) with a hight point in the center.
vtkImageData im
im SetExtent 0 2 0 2 0 2
im SetNumberOfScalarComponents 1
im SetOrigin -1 -1 -1
im AllocateScalars
set s [[im GetPointData] GetScalars]

$s SetScalar 0 0
$s SetScalar 1 0
$s SetScalar 2 0
$s SetScalar 3 0
$s SetScalar 4 0
$s SetScalar 5 0
$s SetScalar 6 0
$s SetScalar 7 0
$s SetScalar 8 0
$s SetScalar 9 0
$s SetScalar 10 0
$s SetScalar 11 0
$s SetScalar 12 0
$s SetScalar 13 1000
$s SetScalar 14 0
$s SetScalar 15 0
$s SetScalar 16 0
$s SetScalar 17 0
$s SetScalar 18 0
$s SetScalar 19 0
$s SetScalar 20 0
$s SetScalar 21 0
$s SetScalar 22 0
$s SetScalar 23 0
$s SetScalar 24 0
$s SetScalar 25 0
$s SetScalar 26 0


# Clip three planes out of the volume.
vtkImageClip clip0
clip0 ClipDataOff
clip0 SetOutputWholeExtent 1 1 0 2 0 2
clip0 SetInput im

vtkImageClip clip1
clip1 ClipDataOff
clip1 SetOutputWholeExtent 0 2 1 1 0 2
clip1 SetInput im

vtkImageClip clip2
clip2 ClipDataOff
clip2 SetOutputWholeExtent 0 2 0 2 1 1
clip2 SetInput im




vtkSynchronizedTemplates2D iso0
    iso0 SetInput [clip0 GetOutput]
    iso0 SetValue 0 500
vtkTubeFilter tuber0
    tuber0 SetInput [iso0 GetOutput]
    tuber0 SetNumberOfSides 8
    tuber0 SetRadius 0.01
vtkPolyDataMapper isoMapper0
    isoMapper0 SetInput [tuber0 GetOutput]
    isoMapper0 ScalarVisibilityOff
vtkActor isoActor0
    isoActor0 SetMapper isoMapper0
    [isoActor0 GetProperty] SetColor 1.0 0 0



vtkSynchronizedTemplates2D iso1
    iso1 SetInput [clip1 GetOutput]
    iso1 SetValue 0 500
vtkTubeFilter tuber1
    tuber1 SetInput [iso1 GetOutput]
    tuber1 SetNumberOfSides 8
    tuber1 SetRadius 0.01
vtkPolyDataMapper isoMapper1
    isoMapper1 SetInput [tuber1 GetOutput]
    isoMapper1 ScalarVisibilityOff
vtkActor isoActor1
    isoActor1 SetMapper isoMapper1
    [isoActor1 GetProperty] SetColor 0 1.0 0



vtkSynchronizedTemplates2D iso2
    iso2 SetInput [clip2 GetOutput]
    iso2 SetValue 0 500
vtkTubeFilter tuber2
    tuber2 SetInput [iso2 GetOutput]
    tuber2 SetNumberOfSides 8
    tuber2 SetRadius 0.01
vtkPolyDataMapper isoMapper2
    isoMapper2 SetInput [tuber2 GetOutput]
    isoMapper2 ScalarVisibilityOff
vtkActor isoActor2
    isoActor2 SetMapper isoMapper2
    [isoActor2 GetProperty] SetColor 0 0 1.0



# Add the actors to the renderer, set the background and size
#
ren1 AddActor isoActor0
ren1 AddActor isoActor1
ren1 AddActor isoActor2

ren1 SetBackground 0.9 0.9 0.9
renWin SetSize 500 500

[ren1 GetActiveCamera] SetPosition 1.2 1.2 1.2
ren1 ResetCameraClippingRange

iren Initialize

iren SetUserMethod {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .


