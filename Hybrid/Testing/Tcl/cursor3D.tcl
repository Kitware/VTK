package require vtk
package require vtkinteraction

# This little example shows how a cursor can be created in 
# image viewers, and renderers.  The standard TkImageViewerWidget and
# TkRenderWidget bindings are used.  There is a new binding:
# middle button in the image viewer sets the position of the cursor.  

# global values
set CURSOR_X 20
set CURSOR_Y 20
set CURSOR_Z 20

set IMAGE_MAG_X 4
set IMAGE_MAG_Y 4
set IMAGE_MAG_Z 1



# Create the GUI: two renderer widgets and a quit button
#
wm withdraw .
toplevel .top 

set help [label .top.help -text "MiddleMouse (or shift-LeftMouse) in image viewer to place cursor"]
set displayFrame [frame .top.f1]
set quitButton [button .top.btn  -text Quit -command exit]
pack $help
pack $displayFrame  -fill both -expand t
pack $quitButton -fill x

set viewerFrame [frame $displayFrame.vFm]
set rendererFrame [frame $displayFrame.rFm]
pack $viewerFrame -padx 3 -pady 3 -side left -fill both -expand f
pack $rendererFrame -padx 3 -pady 3 \
    -side left -fill both -expand t

set viewerWidget [vtkTkImageViewerWidget $viewerFrame.v -width 264 -height 264]
set viewerControls [frame $viewerFrame.c]
pack $viewerControls $viewerWidget -side bottom -fill both -expand f

set downButton [button $viewerControls.down -text "Down" -command "ViewerDown"]
set upButton [button $viewerControls.up -text "Up" -command "ViewerUp"]
set sliceLabel [label $viewerControls.slice \
		    -text "slice: [expr $CURSOR_Z * $IMAGE_MAG_Z]"]
pack $downButton $upButton $sliceLabel -side left -expand t -fill both

vtkRenderWindow renWin
set renderWidget [vtkTkRenderWidget $rendererFrame.r -width 264 -height 264 -rw renWin]
pack $renderWidget -side top


# pipeline stuff
vtkSLCReader reader
    reader SetFileName "$VTK_DATA_ROOT/Data/neghip.slc"

# cursor stuff
vtkImageMagnify magnify
  magnify SetInput [reader GetOutput]
  magnify SetMagnificationFactors $IMAGE_MAG_X $IMAGE_MAG_Y $IMAGE_MAG_Z

vtkImageCursor3D imageCursor
imageCursor SetInput [magnify GetOutput]
imageCursor SetCursorPosition [expr $CURSOR_X * $IMAGE_MAG_X] \
    [expr $CURSOR_Y * $IMAGE_MAG_Y] [expr $CURSOR_Z * $IMAGE_MAG_Z]
imageCursor SetCursorValue 255
imageCursor SetCursorRadius [expr 50 * $IMAGE_MAG_X]

vtkAxes axes
axes SymmetricOn
axes SetOrigin $CURSOR_X $CURSOR_Y $CURSOR_Z 
axes SetScaleFactor 50.0;

vtkPolyDataMapper axesMapper
axesMapper SetInput [axes GetOutput]

vtkActor axesActor
axesActor SetMapper axesMapper
[axesActor GetProperty] SetAmbient 0.5

# image viewer stuff

set viewer [$viewerWidget GetImageViewer]
$viewer SetInput [imageCursor GetOutput]
$viewer SetZSlice [expr $CURSOR_Z * $IMAGE_MAG_Z]
$viewer SetColorWindow 256
$viewer SetColorLevel 128





# Create transfer functions for opacity and color
vtkPiecewiseFunction opacityTransferFunction
    opacityTransferFunction AddPoint  20   0.0
    opacityTransferFunction AddPoint  255  0.2

vtkColorTransferFunction colorTransferFunction
    colorTransferFunction AddRGBPoint 0 0 0 0
    colorTransferFunction AddRGBPoint 64 1 0 0
    colorTransferFunction AddRGBPoint 128 0 0 1
    colorTransferFunction AddRGBPoint 192 0 1 0
    colorTransferFunction AddRGBPoint 255 0 .2 0

# Create properties, mappers, volume actors, and ray cast function
vtkVolumeProperty volumeProperty
    volumeProperty SetColor colorTransferFunction
    volumeProperty SetScalarOpacity opacityTransferFunction

vtkVolumeRayCastCompositeFunction  compositeFunction

vtkVolumeRayCastMapper volumeMapper
    volumeMapper SetInput [reader GetOutput]
    volumeMapper SetVolumeRayCastFunction compositeFunction

vtkVolume volume
    volume SetMapper volumeMapper
    volume SetProperty volumeProperty

# Create outline
vtkOutlineFilter outline
    outline SetInput [reader GetOutput]

vtkPolyDataMapper outlineMapper
    outlineMapper SetInput [outline GetOutput]

vtkActor outlineActor
    outlineActor SetMapper outlineMapper
    eval [outlineActor GetProperty] SetColor 1 1 1



# create the renderer
vtkRenderer ren1
set renWin [$renderWidget GetRenderWindow]
    $renWin AddRenderer ren1
    $renWin SetSize 256 256

ren1 AddActor axesActor
ren1 AddVolume volume
ren1 SetBackground 0.1 0.2 0.4
$renWin Render

proc TkCheckAbort {} {
  global renWin
  set foo [$renWin GetEventPending]
  if {$foo != 0} {$renWin SetAbortRender 1}
}
$renWin SetAbortCheckMethod {TkCheckAbort}



BindTkImageViewer $viewerWidget
BindTkRenderWidget $renderWidget

# lets ass an extra binding of the middle button in the image viewer
# to set the cursor location
bind $viewerWidget <Button-2> {SetCursorFromViewer %x %y}
bind $viewerWidget <Shift-Button-1> {SetCursorFromViewer %x %y}



# supporting procedures



proc ViewerDown {} {
    global viewer
    set z [$viewer GetZSlice]
    ViewerSetZSlice $viewer [expr $z - 1]
}

proc ViewerUp {} {
    global viewer
    set z [$viewer GetZSlice]
    ViewerSetZSlice $viewer [expr $z + 1]
}

proc ViewerSetZSlice {viewer z} {
    global sliceLabel
    $viewer SetZSlice $z
    $sliceLabel configure -text "slice: $z"
    $viewer Render
}


proc SetCursorFromViewer {x y} {
    global viewer viewerWidget
    global IMAGE_MAG_X IMAGE_MAG_Y IMAGE_MAG_Z

    # we have to flip y axis because tk uses upper right origin.
    set height [lindex [$viewerWidget configure -height] 4]
    set y [expr $height - $y]
    set z [$viewer GetZSlice]
    SetCursor [expr $x / $IMAGE_MAG_X] [expr $y / $IMAGE_MAG_Y] \
	[expr $z / $IMAGE_MAG_Z]
}

proc SetCursor {x y z} {
    global viewer renWin
    global CURSOR_X CURSOR_Y CURSOR_Z IMAGE_MAG_X IMAGE_MAG_Y IMAGE_MAG_Z

    set CURSOR_X $x
    set CURSOR_Y $y
    set CURSOR_Z $z
    axes SetOrigin $CURSOR_X $CURSOR_Y $CURSOR_Z 
    imageCursor SetCursorPosition [expr $CURSOR_X * $IMAGE_MAG_X] \
	[expr $CURSOR_Y * $IMAGE_MAG_Y] [expr $CURSOR_Z * $IMAGE_MAG_Z]
    $viewer Render
    $renWin Render
}






