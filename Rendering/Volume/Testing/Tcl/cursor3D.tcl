# This little example shows how a cursor can be created in
# image viewers, and renderers.  The standard TkImageViewerWidget and
# TkRenderWidget bindings are used.  There is a new binding:
# middle button in the image viewer sets the position of the cursor.

# First we include the VTK Tcl packages which will make available
# all of the vtk commands to Tcl

package require vtk
package require vtkinteraction

# Global values

set CURSOR_X 20
set CURSOR_Y 20
set CURSOR_Z 20

set IMAGE_MAG_X 4
set IMAGE_MAG_Y 4
set IMAGE_MAG_Z 1

# Pipeline stuff

vtkSLCReader reader
  reader SetFileName "$VTK_DATA_ROOT/Data/neghip.slc"
# Cursor stuff

vtkImageMagnify magnify
  magnify SetInputConnection [reader GetOutputPort]
  magnify SetMagnificationFactors $IMAGE_MAG_X $IMAGE_MAG_Y $IMAGE_MAG_Z

vtkImageCursor3D image_cursor
  image_cursor SetInputConnection [magnify GetOutputPort]
  image_cursor SetCursorPosition \
          [expr $CURSOR_X * $IMAGE_MAG_X] \
          [expr $CURSOR_Y * $IMAGE_MAG_Y] \
          [expr $CURSOR_Z * $IMAGE_MAG_Z]
  image_cursor SetCursorValue 255
  image_cursor SetCursorRadius [expr 50 * $IMAGE_MAG_X]

vtkAxes axes
  axes SymmetricOn
  axes SetOrigin $CURSOR_X $CURSOR_Y $CURSOR_Z
  axes SetScaleFactor 50.0

vtkPolyDataMapper axes_mapper
  axes_mapper SetInputConnection [axes GetOutputPort]

vtkActor axesActor
  axesActor SetMapper axes_mapper
  [axesActor GetProperty] SetAmbient 0.5

# Image viewer stuff

vtkImageViewer viewer
  viewer SetInputConnection [image_cursor GetOutputPort]
  viewer SetZSlice [expr $CURSOR_Z * $IMAGE_MAG_Z]
  viewer SetColorWindow 256
  viewer SetColorLevel 128

proc viewer_down {viewer} {
    viewer_set_z_slice $viewer [expr [$viewer GetZSlice] - 1]
}

proc viewer_up {viewer} {
    viewer_set_z_slice $viewer [expr [$viewer GetZSlice] + 1]
}

proc viewer_set_z_slice {viewer z} {
    global slice_label
    $viewer SetZSlice $z
    $slice_label configure -text "slice: $z"
    $viewer Render
}

# Create transfer functions for opacity and color

vtkPiecewiseFunction opacity_transfer_function
  opacity_transfer_function AddPoint 20  0.0
  opacity_transfer_function AddPoint 255 0.2

vtkColorTransferFunction color_transfer_function
  color_transfer_function AddRGBPoint 0 0 0 0
  color_transfer_function AddRGBPoint 64 1 0 0
  color_transfer_function AddRGBPoint 128 0 0 1
  color_transfer_function AddRGBPoint 192 0 1 0
  color_transfer_function AddRGBPoint 255 0 .2 0

# Create properties, mappers, volume actors, and ray cast function

vtkVolumeProperty volume_property
  volume_property SetColor color_transfer_function
  volume_property SetScalarOpacity opacity_transfer_function

vtkVolumeRayCastCompositeFunction  composite_function

vtkVolumeRayCastMapper volume_mapper
  volume_mapper SetInputConnection [reader GetOutputPort]
  volume_mapper SetVolumeRayCastFunction composite_function

vtkVolume volume
  volume SetMapper volume_mapper
  volume SetProperty volume_property

# Create outline

vtkOutlineFilter outline
  outline SetInputConnection [reader GetOutputPort]

vtkPolyDataMapper outline_mapper
  outline_mapper SetInputConnection [outline GetOutputPort]

vtkActor outlineActor
  outlineActor SetMapper outline_mapper
  eval [outlineActor GetProperty] SetColor 1 1 1

# Create the renderer

vtkRenderer ren1
  ren1 AddActor axesActor
  ren1 AddVolume volume
  ren1 SetBackground 0.1 0.2 0.4

vtkRenderWindow renWin2
  renWin2 AddRenderer ren1
  renWin2 SetSize 256 256

# Create the GUI: two renderer widgets and a quit button

wm withdraw .
toplevel .top

# Set the window manager (wm command) so that it registers a
# command to handle the WM_DELETE_WINDOW protocal request. This
# request is triggered when the widget is closed using the standard
# window manager icons or buttons. In this case the exit callback
# will be called and it will free up any objects we created then exit
# the application.

wm protocol .top WM_DELETE_WINDOW ::vtk::cb_exit

# Help label, frame and quit button

set help_label [label .top.help \
      -text "MiddleMouse (or shift-LeftMouse) in image viewer to place cursor"]

set display_frame [frame .top.f1]

set quit_button [button .top.btn  \
        -text Quit \
        -command  ::vtk::cb_exit]

# Pack the GUI

pack $help_label

pack $display_frame \
        -fill both -expand t

pack $quit_button \
        -fill x

# Create the viewer widget

set viewer_frame [frame $display_frame.vFm]

pack $viewer_frame \
        -padx 3 -pady 3 \
        -side left -anchor n \
        -fill both -expand f

set viewer_widget [vtkTkImageViewerWidget $viewer_frame.v \
        -width 264 \
        -height 264 \
        -iv viewer]

set viewer_controls [frame $viewer_frame.c]

pack $viewer_widget $viewer_controls  \
        -side top -anchor n \
        -fill both -expand f

set down_button [button $viewer_controls.down \
        -text "Down" \
        -command [list viewer_down viewer]]

set up_button [button $viewer_controls.up \
        -text "Up" \
        -command [list viewer_up viewer]]

set slice_label [label $viewer_controls.slice \
        -text "slice: [expr $CURSOR_Z * $IMAGE_MAG_Z]"]

pack $down_button $up_button $slice_label \
        -side left \
        -expand t -fill both

# Create the render widget

set renderer_frame [frame $display_frame.rFm]

pack $renderer_frame \
        -padx 3 -pady 3 \
        -side left -anchor n \
        -fill both -expand t

set render_widget [vtkTkRenderWidget $renderer_frame.r \
        -width 264 \
        -height 264 \
        -rw renWin2]

pack $render_widget \
        -side top -anchor n \
        -expand t -fill both

# Bindings

::vtk::bind_tk_imageviewer_widget $viewer_widget
$viewer_widget Render

::vtk::bind_tk_render_widget $render_widget
[[[$render_widget GetRenderWindow] GetInteractor] GetInteractorStyle] SetCurrentStyleToTrackballCamera
$render_widget Render

# Lets add an extra binding of the middle button in the image viewer
# to set the cursor location

bind $viewer_widget <Button-2> {SetCursorFromViewer %x %y}
bind $viewer_widget <Shift-Button-1> {SetCursorFromViewer %x %y}

# Supporting procedures

proc SetCursorFromViewer {x y} {
    global viewer_widget
    global IMAGE_MAG_X IMAGE_MAG_Y IMAGE_MAG_Z

    # we have to flip y axis because tk uses upper right origin.
    set height [lindex [$viewer_widget configure -height] 4]
    set y [expr $height - $y]
    set z [viewer GetZSlice]
    SetCursor [expr $x / $IMAGE_MAG_X] [expr $y / $IMAGE_MAG_Y] \
	[expr $z / $IMAGE_MAG_Z]
}

proc SetCursor {x y z} {
    global CURSOR_X CURSOR_Y CURSOR_Z IMAGE_MAG_X IMAGE_MAG_Y IMAGE_MAG_Z

    set CURSOR_X $x
    set CURSOR_Y $y
    set CURSOR_Z $z
    axes SetOrigin $CURSOR_X $CURSOR_Y $CURSOR_Z
    image_cursor SetCursorPosition [expr $CURSOR_X * $IMAGE_MAG_X] \
	[expr $CURSOR_Y * $IMAGE_MAG_Y] [expr $CURSOR_Z * $IMAGE_MAG_Z]
    viewer Render
    renWin2 Render
}
