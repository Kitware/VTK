catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

#
# this test is the same as graphics/examplesTcl/skinOrder.tcl
# except that is uses vtkImageReader, not vtkVolume16Reader
#

# get the interactor ui
source $VTK_TCL/vtkInt.tcl
source $VTK_TCL/colors.tcl
source $VTK_TCL/frog/SliceOrder.tcl

# Create the RenderWindow, Renderer and Interactor
#
vtkRenderer ren1
vtkRenderWindow renWin
  renWin AddRenderer ren1
vtkRenderWindowInteractor iren
  iren SetRenderWindow renWin

set RESOLUTION 64
set START_SLICE 1
set END_SLICE 93
set PIXEL_SIZE 3.2
set centerX [expr ( $RESOLUTION / 2 )]
set centerY [expr ( $RESOLUTION / 2 )]
set centerZ [expr ( $END_SLICE - $START_SLICE ) / 2]
set endX [expr ( $RESOLUTION - 1 ) ]
set endY [expr ( $RESOLUTION - 1 ) ]
set endZ [expr ( $END_SLICE - 1 ) ]
set origin [expr ( $RESOLUTION / 2.0 ) * $PIXEL_SIZE * -1.0]
set VOI "0 [expr $RESOLUTION - 1] 0 [expr $RESOLUTION - 1] $START_SLICE $END_SLICE"

vtkMath math

set orders "ap pa si is lr rl"

foreach order $orders {
  vtkImageReader reader$order
    eval reader$order SetDataExtent $VOI
    reader$order SetFilePrefix $VTK_DATA/headsq/quarter
    reader$order SetDataSpacing $PIXEL_SIZE $PIXEL_SIZE 1.5
    reader$order SetDataOrigin $origin $origin 1.5
    reader$order SetTransform $order
    reader$order SetHeaderSize 0
    reader$order SetDataMask 0x7fff;
    reader$order SetDataByteOrderToLittleEndian
    [reader$order GetOutput] ReleaseDataFlagOn

  vtkContourFilter iso$order
    iso$order SetInput [reader$order GetOutput]
    iso$order SetValue 0 550.5
    iso$order ComputeScalarsOff
    iso$order ReleaseDataFlagOn

  vtkPolyDataMapper mapper$order
    mapper$order SetInput [iso$order GetOutput]
    mapper$order ImmediateModeRenderingOn

  vtkActor actor$order
    actor$order SetMapper mapper$order
    [actor$order GetProperty] SetDiffuseColor [math Random .5 1] [math Random .5 1] [math Random .5 1]

    ren1 AddActor actor$order
}

renWin SetSize 300 300
[ren1 GetActiveCamera] Azimuth 210
[ren1 GetActiveCamera] Elevation 30
[ren1 GetActiveCamera] Dolly 1.2
ren1 ResetCameraClippingRange

ren1 SetBackground .8 .8 .8
iren Initialize;

renWin Render

#renWin SetFileName "skinOrder.tcl.ppm"
#renWin SaveImageAsPPM

iren SetUserMethod {wm deiconify .vtkInteract};

# prevent the tk window from showing up then start the event loop
wm withdraw .
