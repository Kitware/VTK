package require vtk
package require vtkinteraction

# This example demonstrates how to use the vtkImageTracerWidget
# to trace on a slice of a 3D image dataset on one of its orthogonal planes.
# The button actions and key modifiers are as follows for controlling the
# widget:
# 1) left button click over the image, hold and drag draws a free hand line.
# 2) left button click and release erases the widget line, if it exists, and
# repositions the handle.
# 3) middle button click starts a snap line.  The snap line can be
# terminated by clicking the middle button while depressing the ctrl key.
# 4) when tracing or snap drawing a line, if the last cursor position is
# within specified tolerance to the first handle, the widget line will form
# a closed loop with only one handle.
# 5) right button clicking and holding on any handle that is part of a snap
# line allows handle dragging.  Any existing line segments are updated
# accordingly.
# 6) ctrl key + right button down on any handle will erase it. Any existing
# snap line segments are updated accordingly.  If the line was formed by
# continous tracing, the line is deleted leaving one handle.
# 7) shift key + right button down on any snap line segment will insert a
# handle at the cursor position.  The snap line segment is split accordingly.
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

  set range [[v16 GetOutput] GetScalarRange]
  set min [lindex $range 0]
  set max [lindex $range 1]
  set diff [expr $max - $min]
  set slope [expr 255.0/$diff]
  set inter [expr -$slope*$min]
  set shift [expr $inter/$slope]

vtkImageShiftScale shifter
  shifter SetShift $shift
  shifter SetScale $slope
  shifter SetOutputScalarTypeToUnsignedChar
  shifter SetInputConnection [v16 GetOutputPort]
  shifter ReleaseDataFlagOff
  shifter Update

# Display a y-z plane.
#
vtkImageActor imageActor
  [imageActor GetMapper] SetInputConnection [shifter GetOutputPort]
  imageActor VisibilityOn
  imageActor SetDisplayExtent  31 31 0 63 0 92
  imageActor InterpolateOff

  set spc  [[shifter GetOutput] GetSpacing]
  set orig [[shifter GetOutput] GetOrigin]
  set x0   [lindex $orig 0]
  set xspc [lindex $spc 0]
  set pos  [expr $x0 + $xspc*31.0]

# An alternative would be to formulate position in this case by:
# set bounds [imageActor GetBounds]
# set pos [lindex $bounds 0]
#

vtkRenderer ren1
   ren1 SetBackground 0.4 0.4 0.5
vtkRenderer ren2
   ren2 SetBackground 0.5 0.4 0.4

vtkRenderWindow renWin
  renWin AddRenderer ren1
  renWin AddRenderer ren2
  renWin SetSize 600 300

ren1 SetViewport 0 0 0.5 1
ren2 SetViewport 0.5 0 1 1

vtkInteractorStyleImage interactor

vtkRenderWindowInteractor iren
  iren SetInteractorStyle interactor
  iren SetRenderWindow renWin

vtkExtractVOI extract
  extract SetVOI 31 31 0 63 0 92
  extract SetSampleRate 1 1 1
  extract SetInputConnection [shifter GetOutputPort]
  extract ReleaseDataFlagOff
  extract Update

vtkImageActor imageActor2
  [imageActor2 GetMapper] SetInputConnection [extract GetOutputPort]
  imageActor2 VisibilityOn
  imageActor2 SetDisplayExtent  31 31 0 63 0 92
  imageActor2 InterpolateOff

# Set up the image tracer widget
#
vtkImageTracerWidget itw
#
# Set the tolerance for capturing last handle when near first handle
# to form closed paths.
#
  itw SetCaptureRadius 1.5
  [itw GetGlyphSource] SetColor 1 0 0
#
# Set the size of the glyph handle
#
  [itw GetGlyphSource] SetScale 3.0
#
# Set the initial rotation of the glyph if desired.  The default glyph
# set internally by the widget is a '+' so rotating 45 deg. gives a 'x'
#
  [itw GetGlyphSource] SetRotationAngle 45.0
  [itw GetGlyphSource] Modified
  itw ProjectToPlaneOn
  itw SetProjectionNormalToXAxes
  itw SetProjectionPosition $pos
  itw SetViewProp imageActor
  itw SetInputConnection [shifter GetOutputPort]
  itw SetInteractor iren
  itw PlaceWidget
#
# When the underlying vtkDataSet is a vtkImageData, the widget can be
# forced to snap to either nearest pixel points, or pixel centers.  Here
# it is turned off.
#
  itw SnapToImageOff
#
# Automatically form closed paths.
#
  itw AutoCloseOn

# Set up a vtkSplineWidget in the second renderer and have
# its handles set by the tracer widget.
#
vtkSplineWidget isw
  isw SetCurrentRenderer ren2
  isw SetDefaultRenderer ren2
  isw SetInputConnection [extract GetOutputPort]
  isw SetInteractor iren
  set bnds [imageActor2 GetBounds]
  isw PlaceWidget [lindex $bnds 0] [lindex $bnds 1] [lindex $bnds 2] [lindex $bnds 3] [lindex $bnds 4] [lindex $bnds 5]
  isw ProjectToPlaneOn
  isw SetProjectionNormalToXAxes
  isw SetProjectionPosition $pos

# Have the widgets control each others handle positions.
#
  itw AddObserver EndInteractionEvent AdjustSpline
  isw AddObserver EndInteractionEvent AdjustTracer

  itw On
  isw On

vtkPolyData poly
vtkPoints points
vtkPolyData spoly

# Set up a pipleline to demonstrate extraction of a 2D
# region of interest.  Defining a closed clockwise path using the
# tracer widget will extract all pixels within the loop.  A counter
# clockwise path provides the dual region of interest.
#
vtkLinearExtrusionFilter extrude
  extrude SetInputData spoly
  extrude SetScaleFactor 1
  extrude SetExtrusionTypeToNormalExtrusion
  extrude SetVector 1 0 0

# We force the "ribbon" generated by the extrusion to straddle the image plane.
#
vtkTransformPolyDataFilter filter
  filter SetInputConnection [extrude  GetOutputPort]
vtkTransform transform
  transform Translate -0.5 0 0
  filter SetTransform transform

# The spacing, origin and whole extent of the stencil generator must be set.
#
set spc  [[extract GetOutput] GetSpacing]
set orig [[extract GetOutput] GetOrigin]
set wext [[extract GetExecutive] GetWholeExtent [extract GetOutputInformation 0]]

vtkPolyDataToImageStencil dataToStencil
  dataToStencil SetInputConnection [filter GetOutputPort]
  dataToStencil SetOutputSpacing [lindex $spc 0] [lindex $spc 1] [lindex $spc 2]
  dataToStencil SetOutputOrigin [lindex $orig 0] [lindex $orig 1] [lindex $orig 2]
  dataToStencil SetOutputWholeExtent [lindex $wext 0] [lindex $wext 1] [lindex $wext 2] [lindex $wext 3] [lindex $wext 4] [lindex $wext 5]

vtkImageStencil stencil
  stencil SetInputConnection [extract GetOutputPort]
  stencil SetStencilConnection [dataToStencil GetOutputPort]
  stencil ReverseStencilOff
  stencil SetBackgroundValue 128

# Add all the actors.
#
ren1 AddViewProp imageActor
ren2 AddViewProp imageActor2

# Render the image.
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
renWin Render

[ren1 GetActiveCamera] SetViewUp 0 1 0
[ren1 GetActiveCamera] Azimuth 270
[ren1 GetActiveCamera] Roll 270
[ren1 GetActiveCamera] Dolly 1.7
ren1 ResetCameraClippingRange

[ren2 GetActiveCamera] SetViewUp 0 1 0
[ren2 GetActiveCamera] Azimuth 270
[ren2 GetActiveCamera] Roll 270
[ren2 GetActiveCamera] Dolly 1.7
ren2 ResetCameraClippingRange

renWin Render

# Prevent the tk window from showing up then start the event loop.
wm withdraw .

proc AdjustSpline { } {

  set closed [itw IsClosed]

  if { $closed } {
    isw ClosedOn
  } else {
    isw ClosedOff
    [imageActor2 GetMapper] SetInputConnection [extract GetOutputPort]
  }

  set npts [ itw GetNumberOfHandles ]
  if { $npts < 2 } {
    return
  }

  itw GetPath poly
  set pts [poly GetPoints]

  isw InitializeHandles $pts

  if { $closed } {
    isw GetPolyData spoly
    stencil Update
    [imageActor2 GetMapper] SetInputConnection [stencil GetOutputPort]
    }
}

proc AdjustTracer { } {

  set npts [isw GetNumberOfHandles]

  points Reset

  for {set i 0} {$i < $npts} {incr i} {
    set pt [isw GetHandlePosition $i]
    points InsertNextPoint [lindex $pt 0] [lindex $pt 1] [lindex $pt 2]
  }

  set closed [isw IsClosed]

  if { $closed } {
    set ac [itw GetAutoClose]
    if { $ac } {
      set pt [isw GetHandlePosition 0]
      points InsertNextPoint [lindex $pt 0] [lindex $pt 1] [lindex $pt 2]
    }
    isw GetPolyData spoly
    stencil Update
    [imageActor2 GetMapper] SetInputConnection [stencil GetOutputPort]
    } else {
    [imageActor2 GetMapper] SetInputConnection [extract GetOutputPort]
    }

  itw InitializeHandles points
}
