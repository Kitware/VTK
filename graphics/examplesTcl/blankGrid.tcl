catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# Demonstrate how to use structured grid blanking with an image
#

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
vtkExtractGrid plane
    plane SetInput [pl3d GetOutput]
    plane SetVOI 0 57 0 33 0 0
    plane Update
vtkStructuredPoints blankImage
   blankImage SetScalarType 3
   blankImage SetDimensions 57 33 1
set numBlanks [expr 57*33]
vtkScalars blanking
   blanking SetDataTypeToUnsignedChar
   blanking SetNumberOfScalars $numBlanks
for {set i 0} {$i<$numBlanks} {incr i} {
   blanking SetScalar $i 1
}

# Manually blank out areas corresponding to dilution holes
blanking SetScalar 313 0
blanking SetScalar 929 0
blanking SetScalar 1545 0

blanking SetScalar 630 0
blanking SetScalar 1526 0

[blankImage GetPointData] SetScalars blanking

vtkBlankStructuredGridWithImage blankIt
    blankIt SetInput [plane GetOutput]
    blankIt SetBlankingInput blankImage

vtkStructuredGridGeometryFilter blankedPlane
    blankedPlane SetInput [blankIt GetOutput]
    blankedPlane SetExtent 0 100 0 100 0 0

vtkPolyDataMapper planeMapper
    planeMapper SetInput [blankedPlane GetOutput]
    planeMapper SetScalarRange 0.197813 0.710419
vtkActor planeActor
    planeActor SetMapper planeMapper

vtkStructuredGridOutlineFilter outline
    outline SetInput [pl3d GetOutput]
vtkPolyDataMapper outlineMapper
    outlineMapper SetInput [outline GetOutput]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper
    eval [outlineActor GetProperty] SetColor $black

# Add the actors to the renderer, set the background and size
#
ren1 AddActor outlineActor
ren1 AddActor planeActor
ren1 SetBackground 1 1 1
ren1 TwoSidedLightingOff

renWin SetSize 500 500
iren Initialize
iren LightFollowCameraOff

renWin Render
ren1 LightFollowCameraOff

set cam1 [ren1 GetActiveCamera]
    $cam1 SetClippingRange 3.95297 50
    $cam1 SetFocalPoint 8.88908 0.595038 29.3342
    $cam1 SetPosition -12.3332 31.7479 41.2387
    $cam1 SetViewUp 0.060772 -0.319905 0.945498

# render the image
#
renWin Render

iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .



