package require vtk
package require vtkinteraction
package require vtktesting

source [file join [file dirname [info script]] SliceOrder.tcl]

# Create the RenderWindow, Renderer and Interactor
#
vtkRenderer ren1
vtkRenderWindow renWin
  renWin AddRenderer ren1
vtkRenderWindowInteractor iren
  iren SetRenderWindow renWin

set RESOLUTION 64
set START_SLICE 50
set END_SLICE 60
set PIXEL_SIZE 3.2
set centerX [expr ( $RESOLUTION / 2 )]
set centerY [expr ( $RESOLUTION / 2 )]
set centerZ [expr ( $END_SLICE - $START_SLICE ) / 2]
set endX [expr ( $RESOLUTION - 1 ) ]
set endY [expr ( $RESOLUTION - 1 ) ]
set endZ [expr ( $END_SLICE - 1 ) ]
set origin [expr ( $RESOLUTION / 2.0 ) * $PIXEL_SIZE * -1.0]

vtkMath math

set orders "ap pa si is lr rl"

set skinColor_ap "0.875950 0.598302 0.656878"
set skinColor_pa "0.641134 0.536594 0.537889"
set skinColor_si "0.804079 0.650506 0.558249"
set skinColor_is "0.992896 0.603716 0.660385"
set skinColor_lr "0.589101 0.513448 0.523095"
set skinColor_rl "0.650247 0.700527 0.752458"

foreach order $orders {
  vtkVolume16Reader reader$order
    eval reader$order SetDataDimensions $RESOLUTION $RESOLUTION
    reader$order SetFilePrefix $VTK_DATA_ROOT/Data/headsq/quarter
    reader$order SetDataSpacing $PIXEL_SIZE $PIXEL_SIZE 1.5
    reader$order SetDataOrigin $origin $origin 1.5
    reader$order SetImageRange $START_SLICE $END_SLICE
    reader$order SetTransform $order
    reader$order SetHeaderSize 0
    reader$order SetDataMask 0x7fff;
    reader$order SetDataByteOrderToLittleEndian
    [reader$order GetExecutive] SetReleaseDataFlag 0 1

  vtkContourFilter iso$order
    iso$order SetInputConnection [reader$order GetOutputPort]
    iso$order SetValue 0 550.5
    iso$order ComputeScalarsOff
    iso$order ReleaseDataFlagOn

  vtkPolyDataMapper mapper$order
    mapper$order SetInputConnection [iso$order GetOutputPort]
    mapper$order ImmediateModeRenderingOn

  vtkActor actor$order
    actor$order SetMapper mapper$order
    set skinColor [set skinColor_${order}]
    [actor$order GetProperty] SetDiffuseColor [lindex $skinColor 0] [lindex $skinColor 1] [lindex $skinColor 2]

    ren1 AddActor actor$order
}

renWin SetSize 300 300
ren1 ResetCamera
[ren1 GetActiveCamera] Azimuth 210
[ren1 GetActiveCamera] Elevation 30
[ren1 GetActiveCamera] Dolly 1.2
ren1 ResetCameraClippingRange

ren1 SetBackground .8 .8 .8
iren Initialize;

renWin Render

iren AddObserver UserEvent {wm deiconify .vtkInteract};

# prevent the tk window from showing up then start the event loop
wm withdraw .
