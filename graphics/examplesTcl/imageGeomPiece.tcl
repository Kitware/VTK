# This scripts tests a couple of things.
# Images generating ghost level arrays,
# and geometry correctly processing ghost cells.
# It asks for a piece from a geometry filter with an image data input.

catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

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

# create pipeline
#
set RESOLUTION 64
set START_SLICE 1
set END_SLICE 63
set PIXEL_SIZE 3.2

vtkVolume16Reader reader
    eval reader SetDataDimensions $RESOLUTION $RESOLUTION
    reader SetFilePrefix $VTK_DATA/headsq/quarter
    reader SetDataSpacing $PIXEL_SIZE $PIXEL_SIZE 1.5
    reader SetImageRange $START_SLICE $END_SLICE
    reader SetHeaderSize 0
    reader SetDataMask 0x7fff;
    reader SetDataByteOrderToLittleEndian

#vtkImageReader reader
#reader SetDataByteOrderToLittleEndian
#reader SetDataExtent 0 63 0 63 1 64
#reader SetFilePrefix "$VTK_DATA/headsq/quarter"
#reader SetDataMask 0x7fff
#reader SetDataSpacing 1.6 1.6 1.5
#reader SetStartMethod {puts [[reader GetOutput] GetUpdateExtent]}

vtkGeometryFilter geom
geom SetInput [reader GetOutput]

vtkPolyDataMapper mapper
mapper SetInput [geom GetOutput]
mapper ScalarVisibilityOn
mapper SetScalarRange 0 1200
mapper SetPiece 0
mapper SetNumberOfPieces 8

vtkActor actor
actor SetMapper mapper


# Add the actors to the renderer, set the background and size
#
ren1 AddActor actor
ren1 SetBackground 0.2 0.3 0.4
renWin SetSize 450 450
[ren1 GetActiveCamera] Elevation 90
[ren1 GetActiveCamera] SetViewUp 0 0 -1

set cam [ren1 GetActiveCamera]
$cam SetPosition 191.247 -64.5654 285.805
$cam SetViewUp -0.0907763 0.894432 0.437894
$cam SetFocalPoint 49.6 49.6 23.25

ren1 ResetCameraClippingRange
renWin Render

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .


