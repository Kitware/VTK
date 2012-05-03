package require vtk
package require vtkinteraction

# This example demonstrates how to use the vtkImagePlaneWidget
# to probe a 3D image dataset with three orthogonal planes.
# Buttons are provided to:
# a) capture the render window display to a tiff file
# b) x,y,z buttons reset the widget to orthonormal
#    positioning, set the horizontal slider to move the
#    associated widget along its normal, and set the
#    camera to face the widget
# c) right clicking on x,y,z buttons pops up a menu to set
#    the associated widget's reslice interpolation mode
#

# Start by loading some data.
#
vtkVolume16Reader v16
  v16 SetDataDimensions 64 64
  v16 SetDataByteOrderToLittleEndian
  v16 SetFilePrefix "$VTK_DATA_ROOT/Data/headsq/quarter"
  v16 SetImageRange 1 93
  v16 SetDataSpacing 3.2 3.2 1.5
  v16 Update

scan [[v16 GetExecutive] GetWholeExtent [v16 GetOutputInformation 0]] "%d %d %d %d %d %d" \
        xMin xMax yMin yMax zMin zMax

set spacing [[v16 GetOutput] GetSpacing]
set sx [lindex $spacing 0]
set sy [lindex $spacing 1]
set sz [lindex $spacing 2]

set origin [[v16 GetOutput] GetOrigin]
set ox [lindex $origin 0]
set oy [lindex $origin 1]
set oz [lindex $origin 2]

# An outline is shown for context.
#
vtkOutlineFilter outline
  outline SetInputConnection [v16 GetOutputPort]

vtkPolyDataMapper outlineMapper
  outlineMapper SetInputConnection [outline GetOutputPort]

vtkActor outlineActor
  outlineActor SetMapper outlineMapper

# The shared picker enables us to use 3 planes at one time
# and gets the picking order right
#
vtkCellPicker picker
  picker SetTolerance 0.005

# The 3 image plane widgets are used to probe the dataset.
#
vtkImagePlaneWidget planeWidgetX
  planeWidgetX DisplayTextOn
  planeWidgetX SetInputConnection [v16 GetOutputPort]
  planeWidgetX SetPlaneOrientationToXAxes
  planeWidgetX SetSliceIndex 32
  planeWidgetX SetPicker picker
  planeWidgetX SetKeyPressActivationValue "x"
  set prop1 [planeWidgetX GetPlaneProperty]
  $prop1 SetColor 1 0 0

vtkImagePlaneWidget planeWidgetY
  planeWidgetY DisplayTextOn
  planeWidgetY SetInputConnection [v16 GetOutputPort]
  planeWidgetY SetPlaneOrientationToYAxes
  planeWidgetY SetSliceIndex 32
  planeWidgetY SetPicker picker
  planeWidgetY SetKeyPressActivationValue "y"
  set prop2 [planeWidgetY GetPlaneProperty]
  $prop2 SetColor 1 1 0
  planeWidgetY SetLookupTable [planeWidgetX GetLookupTable]

# for the z-slice, turn off texture interpolation:
# interpolation is now nearest neighbour, to demonstrate
# cross-hair cursor snapping to pixel centers
#
vtkImagePlaneWidget planeWidgetZ
  planeWidgetZ DisplayTextOn
  planeWidgetZ SetInputConnection [v16 GetOutputPort]
  planeWidgetZ SetPlaneOrientationToZAxes
  planeWidgetZ SetSliceIndex 46
  planeWidgetZ SetPicker picker
  planeWidgetZ SetKeyPressActivationValue "z"
  set prop3 [planeWidgetZ GetPlaneProperty]
  $prop3 SetColor 0 0 1
  planeWidgetZ SetLookupTable [planeWidgetX GetLookupTable]

set current_widget planeWidgetZ
set mode_widget  planeWidgetZ

planeWidgetX AddObserver EndWindowLevelEvent WindowLevelXCallback
planeWidgetY AddObserver EndWindowLevelEvent WindowLevelYCallback
planeWidgetZ AddObserver EndWindowLevelEvent WindowLevelZCallback
planeWidgetX AddObserver ResetWindowLevelEvent WindowLevelXCallback
planeWidgetY AddObserver ResetWindowLevelEvent WindowLevelYCallback
planeWidgetZ AddObserver ResetWindowLevelEvent WindowLevelZCallback

# Create the RenderWindow and Renderer
#
vtkRenderer ren1
vtkRenderWindow renWin
  renWin AddRenderer ren1

# Add the outline actor to the renderer, set the background color and size
#
ren1 AddActor outlineActor
renWin SetSize 600 600
ren1 SetBackground  0.1 0.1 0.2


# Create the GUI
#
wm withdraw .
toplevel .top
wm title .top "vtkImagePlaneWidget Example"
wm protocol .top WM_DELETE_WINDOW ::vtk::cb_exit

set popm [menu .top.mm -tearoff 0]
set mode 1
$popm add radiobutton -label "nearest" -variable mode -value 0  \
           -command SetInterpolation
$popm add radiobutton -label "linear" -variable mode -value 1  \
           -command SetInterpolation
$popm add radiobutton -label "cubic" -variable mode -value 2  \
           -command SetInterpolation

set display_frame [frame .top.f1]

set ctrl_buttons [frame .top.btns]

pack $display_frame $ctrl_buttons \
        -side top -anchor n \
        -fill both -expand f

set quit_button [button $ctrl_buttons.btn1  \
        -text "Quit" \
        -command  ::vtk::cb_exit]

set capture_button [button $ctrl_buttons.btn2  \
        -text "Tif" \
        -command CaptureImage]

set x_button [button $ctrl_buttons.btn3  \
        -text "x" \
        -command AlignXaxis]

set y_button [button $ctrl_buttons.btn4  \
        -text "y" \
        -command AlignYaxis]

set z_button [button $ctrl_buttons.btn5  \
        -text "z" \
        -command AlignZaxis]

set last_btn -1
bind $x_button <Button-3> "set last_btn 0; configMenu; $popm post %X %Y"
bind $y_button <Button-3> "set last_btn 1; configMenu; $popm post %X %Y"
bind $z_button <Button-3> "set last_btn 2; configMenu; $popm post %X %Y"

# Share the popup menu among buttons, keeping
# track of associated widget's interpolation mode
#
proc configMenu { } {
  global last_btn popm mode mode_widget
  if { $last_btn == 0 } {
    set mode_widget planeWidgetX
  } elseif { $last_btn == 1 } {
    set mode_widget planeWidgetY
  } else {
    set mode_widget planeWidgetZ
  }
  set mode [$mode_widget GetResliceInterpolate]
  $popm entryconfigure $last_btn -variable mode
}

pack $quit_button $capture_button $x_button $y_button $z_button \
        -side left \
        -expand t -fill both

# Create the render widget
#
set renderer_frame [frame $display_frame.rFm]

pack $renderer_frame \
        -padx 3 -pady 3 \
        -side left -anchor n \
        -fill both -expand f

set render_widget [vtkTkRenderWidget $renderer_frame.r \
        -width 600 \
        -height 600 \
        -rw renWin]

pack $render_widget $display_frame  \
        -side top -anchor n \
        -fill both -expand f

# Add a slice scale to browse the current slice stack
#

set slice_number [$current_widget GetSliceIndex]

scale .top.slice \
        -from $zMin \
        -to $zMax \
        -orient horizontal \
        -command SetSlice \
        -variable slice_number \
        -label "Slice"

pack .top.slice \
        -fill x -expand f

proc SetSlice {slice} {
  global current_widget
  $current_widget SetSliceIndex $slice
  ren1 ResetCameraClippingRange
  renWin Render
}

::vtk::bind_tk_render_widget $render_widget
# Set the interactor for the widgets
#
set iact [[$render_widget GetRenderWindow] GetInteractor]
planeWidgetX SetInteractor $iact
planeWidgetX On
planeWidgetY SetInteractor $iact
planeWidgetY On
planeWidgetZ SetInteractor $iact
planeWidgetZ On

# Create an initial interesting view
#
set cam1 [ren1 GetActiveCamera]
$cam1 Elevation 110
$cam1 SetViewUp 0 0 -1
$cam1 Azimuth 45
ren1 ResetCameraClippingRange

# Render it
#
$render_widget Render

# Supporting procedures
#

# Align the camera so that it faces the desired widget
#
proc AlignCamera { } {
  global ox oy oz sx sy sz xMax xMin yMax yMin zMax zMin slice_number
  global current_widget
  set cx [expr $ox + (0.5*($xMax - $xMin))*$sx]
  set cy [expr $oy + (0.5*($yMax - $yMin))*$sy]
  set cz [expr $oy + (0.5*($zMax - $zMin))*$sz]
  set vx 0
  set vy 0
  set vz 0
  set nx 0
  set ny 0
  set nz 0
  set iaxis [$current_widget GetPlaneOrientation]
  if { $iaxis == 0 } {
    set vz -1
    set nx [expr $ox + $xMax*$sx]
    set cx [expr $ox + $slice_number*$sx]
  }  elseif  { $iaxis == 1 } {
    set vz -1
    set ny [expr $oy + $yMax*$sy]
    set cy [expr $oy + $slice_number*$sy]
  } else {
    set vy 1
    set nz [expr $oz + $zMax*$sz]
    set cz [expr $oz + $slice_number*$sz]
  }
  set px [expr $cx + $nx*2]
  set py [expr $cy + $ny*2]
  set pz [expr $cz + $nz*3]

  set camera [ ren1 GetActiveCamera ]
  $camera SetViewUp $vx $vy $vz
  $camera SetFocalPoint $cx $cy $cz
  $camera SetPosition $px $py $pz
  $camera OrthogonalizeViewUp
  ren1 ResetCameraClippingRange
  renWin Render
}

# Capture the display and place in a tiff
#
proc CaptureImage { } {
  vtkWindowToImageFilter w2i
  vtkTIFFWriter writer

  w2i SetInput renWin
  w2i Update
  writer SetInputConnection [w2i GetOutputPort]
  writer SetFileName image.tif
  renWin Render
  writer Write

  writer Delete
  w2i Delete
}

# Align the widget back into orthonormal position,
# set the slider to reflect the widget's position,
# call AlignCamera to set the camera facing the widget
#
proc AlignXaxis { } {
  global xMax xMin current_widget slice_number
  set po [ planeWidgetX GetPlaneOrientation ]
  if { $po == 3 } {
    planeWidgetX SetPlaneOrientationToXAxes
    set slice_number [expr ($xMax - $xMin)/2]
    planeWidgetX SetSliceIndex $slice_number
  } else {
    set slice_number [planeWidgetX GetSliceIndex]
  }
  set current_widget planeWidgetX
  .top.slice config -from $xMin -to $xMax
  .top.slice set $slice_number
  AlignCamera
}

proc AlignYaxis { } {
  global yMin yMax current_widget slice_number
  set po [ planeWidgetY GetPlaneOrientation ]
  if { $po == 3 } {
    planeWidgetY SetPlaneOrientationToYAxes
    set slice_number [expr ($yMax - $yMin)/2]
    planeWidgetY SetSliceIndex $slice_number
  } else {
    set slice_number [planeWidgetY GetSliceIndex]
  }
  set current_widget planeWidgetY
  .top.slice config -from $yMin -to $yMax
  .top.slice set $slice_number
  AlignCamera
}

proc AlignZaxis { } {
  global zMin zMax current_widget slice_number
  set po [ planeWidgetZ GetPlaneOrientation ]
  if { $po == 3 } {
    planeWidgetZ SetPlaneOrientationToZAxes
    set slice_number [expr ($zMax - $zMin)/2]
    planeWidgetZ SetSliceIndex $slice_number
  } else {
    set slice_number [planeWidgetZ GetSliceIndex]
  }
  set current_widget planeWidgetZ
  .top.slice config -from $zMin -to $zMax
  .top.slice set $slice_number
  AlignCamera
}

# Set the widget's reslice interpolation mode
# to the corresponding popup menu choice
#
proc SetInterpolation { } {
  global mode_widget mode
  if { $mode == 0 } {
    $mode_widget TextureInterpolateOff
  } else {
    $mode_widget TextureInterpolateOn
  }
  $mode_widget SetResliceInterpolate $mode
  renWin Render
}

proc WindowLevelXCallback {} {
  set w [planeWidgetX GetWindow]
  set l [planeWidgetX GetLevel]
  planeWidgetY SetWindowLevel $w $l 1
  planeWidgetZ SetWindowLevel $w $l 1
}

proc WindowLevelYCallback {} {
  set w [planeWidgetY GetWindow]
  set l [planeWidgetY GetLevel]
  planeWidgetX SetWindowLevel $w $l 1
  planeWidgetZ SetWindowLevel $w $l 1
}

proc WindowLevelZCallback {} {
  set w [planeWidgetZ GetWindow]
  set l [planeWidgetZ GetLevel]
  planeWidgetX SetWindowLevel $w $l 1
  planeWidgetY SetWindowLevel $w $l 1
}





