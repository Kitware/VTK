package require vtk
package require vtkinteraction

# This example demonstrates how to use the vtkImagePlaneWidget
# and a vtkSplineWidger to do profile probing of a 3D image
# dataset.  The use of vtkAnnotatedCubeActor, vtkAxesActor and
# vtkOrientationMarkerWidget is also demostrated.
#
# GUI controls are provided as follows:
# a) x,y,z buttons set the widgets to orthonormal
#    positioning, set the horizontal slider to move the
#    widgets along their common plane normal, and set the
#    camera to face the widgets
# b) right clicking on x,y,z buttons pops up a menu to set
#    the widget's reslice interpolation mode
# c) when in axes aligned, orthogonal orientation, the slider
#    will move the widget by slice index within the appropriate range
#

# Start by loading some data.
#
vtkVolume16Reader v16
  v16 SetDataDimensions 64 64
  v16 SetDataByteOrderToLittleEndian
  v16 SetImageRange 1 93
  v16 SetDataSpacing 3.2 3.2 1.5
  v16 SetFilePrefix "$VTK_DATA_ROOT/Data/headsq/quarter"
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

# Create an outline of the 3D image data bounds.
#
vtkOutlineFilter outline
  outline SetInputConnection [ v16 GetOutputPort ]

vtkPolyDataMapper outlineMapper
  outlineMapper SetInputConnection [ outline GetOutputPort ]

vtkActor outlineActor
  outlineActor SetMapper outlineMapper

# Set up two renderers in one render window: one for the
# vtkImagePlaneWidget, vtkSplineWidget, and outline, and
# one for the profile plot.
#
vtkRenderer ren1
   ren1 SetBackground 0.4 0.4 0.5
vtkRenderer ren2
   ren2 SetBackground 0.8 0.8 0.8

vtkRenderWindow renWin
  renWin AddRenderer ren1
  renWin AddRenderer ren2
  renWin SetSize 800 400

ren1 SetViewport 0 0 0.5 1
ren2 SetViewport 0.5 0 1 1

# Create a vtkImagePlaneWidget to slice through the data.
#
vtkImagePlaneWidget ipw
  ipw DisplayTextOn
  ipw TextureInterpolateOff
  ipw UserControlledLookupTableOff
  ipw SetInputConnection [ v16 GetOutputPort ]
  ipw SetResliceInterpolateToNearestNeighbour
  ipw KeyPressActivationOff
  [ ipw GetPlaneProperty ] SetColor 1 0 0
  set xmode [ ipw GetResliceInterpolate ]
  set ymode [ ipw GetResliceInterpolate ]
  set zmode [ ipw GetResliceInterpolate ]
  ipw SetPlaneOrientationToXAxes
  ipw SetSliceIndex 32
  ipw AddObserver InteractionEvent UpdateIPW

# Create a vtkSplineWidget to interactively probe the data.
#
vtkSplineWidget spline
  spline SetInputConnection [ v16 GetOutputPort ]
  spline PlaceWidget
  spline SetPriority 1.0
  spline KeyPressActivationOff
  spline ProjectToPlaneOn
  spline SetProjectionNormal 0
  spline SetProjectionPosition 102.4
  spline SetNumberOfHandles 5
  spline SetResolution 500
  spline AddObserver InteractionEvent UpdateSW

# A vtkPolyData will be continuously updated from the spline
# during interaction.
#
vtkPolyData poly
  spline GetPolyData poly

# The filter to probe the image data.
#
vtkProbeFilter probe
  probe SetInputData poly
  probe SetSourceConnection [ v16 GetOutputPort ]

# The plot of the profile data.
#
vtkXYPlotActor profile
  profile AddDataSetInputConnection [ probe GetOutputPort ]
  [ profile GetPositionCoordinate ] SetValue 0.05 0.05 0
  [ profile GetPosition2Coordinate ] SetValue 0.95 0.95 0
  profile SetXValuesToNormalizedArcLength
  profile SetNumberOfXLabels 6
  profile SetTitle "Profile Data "
  profile SetXTitle "s"
  profile SetYTitle "I(s)"
  profile SetXRange 0 1
  set range [[v16 GetOutput] GetScalarRange]
  profile SetYRange [lindex $range 0] [lindex $range 1]
  [ profile GetProperty ] SetColor 0 0 0
  [ profile GetProperty ] SetLineWidth  2
  profile SetLabelFormat "%g"
  [ profile GetTitleTextProperty ] SetColor 0.02 0.06 0.62
  [ profile GetTitleTextProperty ] SetFontFamilyToArial
  profile SetAxisTitleTextProperty [ profile GetTitleTextProperty ]
  profile SetAxisLabelTextProperty [ profile GetTitleTextProperty ]
  profile SetTitleTextProperty [ profile GetTitleTextProperty ]

# Create a composite orientation marker using
# vtkAnnotatedCubeActor and vtkAxesActor.
#
vtkAnnotatedCubeActor cube
  cube SetXPlusFaceText  "R"
  cube SetXMinusFaceText "L"
  cube SetYPlusFaceText  "A"
  cube SetYMinusFaceText "P"
  cube SetZPlusFaceText  "I"
  cube SetZMinusFaceText "S"
  cube SetXFaceTextRotation 180
  cube SetYFaceTextRotation 180
  cube SetZFaceTextRotation -90
  cube SetFaceTextScale 0.65
  set property [ cube GetCubeProperty ]
  $property SetColor 0.5 1 1
  set property [ cube GetTextEdgesProperty ]
  $property SetLineWidth 1
  $property SetDiffuse 0
  $property SetAmbient 1
  $property SetColor 0.18 0.28 0.23
  set property [ cube GetXPlusFaceProperty ]
  $property SetColor 0 0 1
  $property SetInterpolationToFlat
  set property [ cube GetXMinusFaceProperty ]
  $property SetColor 0 0 1
  $property SetInterpolationToFlat
  set property [ cube GetYPlusFaceProperty ]
  $property SetColor 0 1 0
  $property SetInterpolationToFlat
  set property [ cube GetYMinusFaceProperty ]
  $property SetColor 0 1 0
  $property SetInterpolationToFlat
  set property [ cube GetZPlusFaceProperty ]
  $property SetColor 1 0 0
  $property SetInterpolationToFlat
  set property [ cube GetZMinusFaceProperty ]
  $property SetColor 1 0 0
  $property SetInterpolationToFlat

vtkAxesActor axes
  axes SetShaftTypeToCylinder
  axes SetXAxisLabelText "x"
  axes SetYAxisLabelText "y"
  axes SetZAxisLabelText "z"
  axes SetTotalLength 1.5 1.5 1.5
  vtkTextProperty tprop
  tprop ItalicOn
  tprop ShadowOn
  tprop SetFontFamilyToTimes
  [ axes GetXAxisCaptionActor2D ] SetCaptionTextProperty tprop
  vtkTextProperty tprop2
  tprop2 ShallowCopy tprop
  [ axes GetYAxisCaptionActor2D ] SetCaptionTextProperty tprop2
  vtkTextProperty tprop3
  tprop3 ShallowCopy tprop
  [ axes GetZAxisCaptionActor2D ] SetCaptionTextProperty tprop3

# Combine the two actors into one with vtkPropAssembly ...
#
  vtkPropAssembly assembly
  assembly AddPart axes
  assembly AddPart cube

# Add the composite marker to the widget.  The widget
# should be kept in non-interactive mode and the aspect
# ratio of the render window taken into account explicitly,
# since the widget currently does not take this into
# account in a multi-renderer environment.
#
  vtkOrientationMarkerWidget marker
  marker SetOutlineColor 0.93 0.57 0.13
  marker SetOrientationMarker assembly
  marker SetViewport 0.0 0.0 0.15 0.3

# Add the actors.
#
  ren1 AddActor outlineActor
  ren2 AddActor2D profile

# Prevent the tk window from showing up then start the event loop.
  wm withdraw .

# Build a tcl GUI.
#

toplevel .top
wm title .top "Probe With vtkSplineWidget Example"
wm protocol .top WM_DELETE_WINDOW ::vtk::cb_exit

set popm [menu .top.mm -tearoff 0]
set interpmode 0
$popm add radiobutton -label "nearest" -variable interpmode -value 0  \
           -command SetInterpolation
$popm add radiobutton -label "linear" -variable interpmode -value 1  \
           -command SetInterpolation
$popm add radiobutton -label "cubic" -variable interpmode -value 2  \
           -command SetInterpolation

set display_frame [frame .top.f1]

set ctrl_buttons [frame .top.btns]

pack $display_frame $ctrl_buttons \
        -side top -anchor n \
        -fill both -expand f

set quit_button [button $ctrl_buttons.btn1  \
        -text "Quit" \
        -command  ::vtk::cb_exit]

set x_button [button $ctrl_buttons.btn2  \
        -text "x" \
        -command AlignXaxis]

set y_button [button $ctrl_buttons.btn3  \
        -text "y" \
        -command AlignYaxis]

set z_button [button $ctrl_buttons.btn4  \
        -text "z" \
        -command AlignZaxis]

set last_btn -1
bind $x_button <Button-3> "set last_btn 0; ConfigMenu; $popm post %X %Y"
bind $y_button <Button-3> "set last_btn 1; ConfigMenu; $popm post %X %Y"
bind $z_button <Button-3> "set last_btn 2; ConfigMenu; $popm post %X %Y"

# Share the popup menu among buttons, keeping
# track of associated plane's interpolation mode
#
proc ConfigMenu { } {
  global last_btn popm interpmode xmode ymode zmode
  if { $last_btn == 0 } {
    set interpmode $xmode
   } elseif { $last_btn == 1 } {
    set interpmode $ymode
  } else {
    set interpmode $zmode
  }
  $popm entryconfigure $last_btn -variable interpmode
}

pack $quit_button $x_button $y_button $z_button \
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
        -width 800 \
        -height 400 \
        -rw renWin]

pack $render_widget $display_frame  \
        -side top -anchor n \
        -fill both -expand f

# Add a slice scale to browse the current slice stack
#

set slice_number [ipw GetSliceIndex]

scale .top.slice \
        -from $xMin \
        -to $xMax \
        -orient horizontal \
        -command SetSlice \
        -variable slice_number \
        -label "Slice"

pack .top.slice \
        -fill x -expand f

proc SetSlice {slice} {
  ipw SetSliceIndex $slice
  UpdateIPW
  renWin Render
}

::vtk::bind_tk_render_widget $render_widget
# Set the interactor for the widgets
#
set iact [[$render_widget GetRenderWindow] GetInteractor]
vtkInteractorStyleTrackballCamera style
$iact SetInteractorStyle style

ipw SetInteractor $iact
ipw On

spline SetInteractor $iact
spline SetPlaneSource [ ipw GetPolyDataAlgorithm ]
spline SetProjectionNormal 3
spline On

marker SetInteractor $iact
marker SetEnabled 1
marker InteractiveOff

# Create an initial interesting view
#
set cam1 [ren1 GetActiveCamera]
$cam1 Elevation 110
$cam1 SetViewUp 0 0 -1
$cam1 Azimuth 45
ren1 ResetCamera

# Render it
#
$render_widget Render

# Supporting procedures
#

# Align the camera so that it faces the desired widget
#
proc AlignCamera { } {
  global ox oy oz sx sy sz xMax xMin yMax yMin zMax zMin slice_number
  set cx [expr $ox + (0.5*($xMax - $xMin))*$sx]
  set cy [expr $oy + (0.5*($yMax - $yMin))*$sy]
  set cz [expr $oy + (0.5*($zMax - $zMin))*$sz]
  set vx 0
  set vy 0
  set vz 0
  set nx 0
  set ny 0
  set nz 0
  set iaxis [ipw GetPlaneOrientation]
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
  ren1 ResetCamera
  renWin Render
}

# Align the widget back into orthonormal position,
# set the slider to reflect the widget's position,
# call AlignCamera to set the camera facing the widget
#
proc AlignXaxis { } {
  global xMax xMin slice_number interpmode xmode
  set ori [ ipw GetPlaneOrientation ]
  if { $ori != 0 } {
    ipw SetPlaneOrientationToXAxes
    set slice_number [expr ($xMax - $xMin)/2]
    ipw SetSliceIndex $slice_number
  } else {
    set slice_number [ipw GetSliceIndex]
  }
  .top.slice config -from $xMin -to $xMax
  .top.slice set $slice_number
  UpdateSplinePosition
  AlignCamera
  set interpmode $xmode
  SetInterpolation
}

proc AlignYaxis { } {
  global yMin yMax slice_number interpmode ymode

  set po [ ipw GetPlaneOrientation ]
  set ori [ ipw GetPlaneOrientation ]
  if { $ori != 1 } {
    ipw SetPlaneOrientationToYAxes
    set slice_number [expr ($yMax - $yMin)/2]
    ipw SetSliceIndex $slice_number
  } else {
    set slice_number [ipw GetSliceIndex]
  }
  .top.slice config -from $yMin -to $yMax
  .top.slice set $slice_number
  UpdateSplinePosition
  AlignCamera
  set interpmode $ymode
  SetInterpolation
}

proc AlignZaxis { } {
  global zMin zMax slice_number interpmode zmode
  set ori [ ipw GetPlaneOrientation ]
  if { $ori != 2 } {
    ipw SetPlaneOrientationToZAxes
    set slice_number [expr ($zMax - $zMin)/2]
    ipw SetSliceIndex $slice_number
  } else {
    set slice_number [ipw GetSliceIndex]
  }
  .top.slice config -from $zMin -to $zMax
  .top.slice set $slice_number
  UpdateSplinePosition
  AlignCamera
  set interpmode $zmode
  SetInterpolation
}

proc UpdateSplinePosition { } {
  set ori [ ipw GetPlaneOrientation ]
  spline ProjectToPlaneOff
  spline PlaceWidget
  spline ProjectToPlaneOn
  spline SetProjectionNormal $ori
  spline SetProjectionNormal 3
  UpdateIPW
}

# Set the vtkImagePlaneWidget's reslice interpolation mode
# to the corresponding popup menu choice and store
# it for subsequent orientation changes.
#
proc SetInterpolation { } {
  global interpmode xmode ymode zmode
  if { $interpmode == 0 } {
    ipw TextureInterpolateOff
  } else {
    ipw TextureInterpolateOn
  }
  ipw SetResliceInterpolate $interpmode
  set ori [ipw GetPlaneOrientation]
  if { $ori == 0 } {
    set xmode $interpmode
  }  elseif  { $ori == 1 } {
    set ymode $interpmode
  } else {
    set zmode $interpmode
  }
  renWin Render
}

# Procedure to update the placement of the vtkSplineWidget on the
# plane defined by the vtkImagePlaneWidget.
#
proc UpdateIPW { } {
  set ori [ ipw GetPlaneOrientation ]

  if { $ori == 3 } {
    spline SetProjectionPosition 0
  } else {
    set pos [ ipw GetSlicePosition ]
    spline SetProjectionPosition $pos
  }

  UpdateSW
}

# Procedure to update the spline geometry fed into the probe filter.
#
proc UpdateSW { } {
  spline GetPolyData poly
}

