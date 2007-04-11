package require vtk
package require vtkinteraction

# This example shows how to load a 3D image into VTK and then reformat
# that image into a different orientation for viewing.  It uses
# vtkImageReslice for reformatting the image, and uses vtkImageActor
# and vtkInteractorStyleImage to display the image.  This InteractorStyle
# forces the camera to stay perpendicular to the XY plane.

# Start by loading some data.
vtkImageReader2 reader
  reader SetFilePrefix "$VTK_DATA_ROOT/Data/headsq/quarter"
  reader SetDataExtent 0 63 0 63 1 93
  reader SetDataSpacing 3.2 3.2 1.5
  reader SetDataOrigin 0.0 0.0 0.0
  reader SetDataScalarTypeToUnsignedShort
  reader UpdateWholeExtent

# Calculate the center of the volume
[reader GetOutput] UpdateInformation
set extent [[reader GetOutput] GetWholeExtent]
set spacing [[reader GetOutput] GetSpacing]
set origin [[reader GetOutput] GetOrigin]

set xMin [lindex $extent 0]
set xMax [lindex $extent 1]
set yMin [lindex $extent 2]
set yMax [lindex $extent 3]
set zMin [lindex $extent 4]
set zMax [lindex $extent 5]

set xSpacing [lindex $spacing 0]
set ySpacing [lindex $spacing 1]
set zSpacing [lindex $spacing 2]

set x0 [lindex $origin 0]
set y0 [lindex $origin 1]
set z0 [lindex $origin 2]

set xCenter [expr $x0 + $xSpacing * 0.5 * ($xMin + $xMax)]
set yCenter [expr $y0 + $ySpacing * 0.5 * ($yMin + $yMax)]
set zCenter [expr $z0 + $zSpacing * 0.5 * ($zMin + $zMax)]

# Matrices for axial, coronal, sagittal, oblique view orientations
vtkMatrix4x4 axial
set elements { 1  0  0  $xCenter
               0  1  0  $yCenter
               0  0  1  $zCenter
               0  0  0  1}
for {set i 0} {$i < 16} {incr i} {
  axial SetElement [expr $i / 4] [expr $i % 4] [expr [lindex $elements $i]] 
}

vtkMatrix4x4 coronal
set elements { 1  0  0  $xCenter
               0  0  1  $yCenter
               0 -1  0  $zCenter
               0  0  0  1}
for {set i 0} {$i < 16} {incr i} {
  coronal SetElement [expr $i / 4] [expr $i % 4] [expr [lindex $elements $i]] 
}

vtkMatrix4x4 sagittal
set elements { 0  0 -1  $xCenter
               1  0  0  $yCenter
               0 -1  0  $zCenter
               0  0  0  1}
for {set i 0} {$i < 16} {incr i} {
  sagittal SetElement [expr $i / 4] [expr $i % 4] [expr [lindex $elements $i]] 
}


vtkMatrix4x4 oblique
set elements { 1  0  0  $xCenter
               0  0.866025 -0.5  $yCenter
               0  0.5  0.866025  $zCenter
               0  0  0  1 }
for {set i 0} {$i < 16} {incr i} {
  oblique SetElement [expr $i / 4] [expr $i % 4] [expr [lindex $elements $i]] 
}

# Extract a slice in the desired orientation
vtkImageReslice reslice
  reslice SetInputConnection [reader GetOutputPort]
  reslice SetOutputDimensionality 2 
  reslice SetResliceAxes sagittal 
  reslice SetInterpolationModeToLinear

# Create a greyscale lookup table
vtkLookupTable table
  table SetTableRange 0 2000 
  table SetValueRange 0.0 1.0
  table SetSaturationRange 0.0 0.0
  table SetRampToLinear
  table Build

# Map the image through the lookup table
vtkImageMapToColors color
  color SetLookupTable table
  color SetInputConnection [reslice GetOutputPort]

# Display the image
vtkImageActor actor
  actor SetInput [color GetOutput]

vtkRenderer renderer
  renderer AddActor actor

vtkRenderWindow window
  window AddRenderer renderer

# Set up the interaction
vtkInteractorStyleImage imageStyle

vtkRenderWindowInteractor interactor
  interactor SetInteractorStyle imageStyle
  window SetInteractor interactor 
  window Render

# Create callbacks for slicing the image
global action
set action ""

proc ButtonPressCallback {} {
    global action
    set action "Slicing"
}

proc ButtonReleaseCallback {} {
    global action
    set action ""
}

proc MouseMoveCallback {} {
    set lastPos [interactor GetLastEventPosition]
    set currPos [interactor GetEventPosition]
    global action
    if {$action == "Slicing"} {
        set deltaY [expr [lindex $currPos 1] - [lindex $lastPos 1]]
        [reslice GetOutput] UpdateInformation
        set spacing [[reslice GetOutput] GetSpacing]
        set sliceSpacing [lindex $spacing 2]
        set matrix [reslice GetResliceAxes]
        # move the center point that we are slicing through
        set center [$matrix MultiplyPoint 0 0 [expr $sliceSpacing * $deltaY] 1]
        $matrix SetElement 0 3 [lindex $center 0]
        $matrix SetElement 1 3 [lindex $center 1]
        $matrix SetElement 2 3 [lindex $center 2]
        window Render
    } else {
        imageStyle OnMouseMove
    }
}

imageStyle AddObserver MouseMoveEvent MouseMoveCallback
imageStyle AddObserver LeftButtonPressEvent ButtonPressCallback
imageStyle AddObserver LeftButtonReleaseEvent ButtonReleaseCallback

interactor AddObserver UserEvent {wm deiconify .vtkInteract}
interactor AddObserver ExitEvent {exit}
interactor Initialize

#
# Hide the default . widget
#
wm withdraw .

#
# You only need this line if you run this script from a Tcl shell
# (tclsh) instead of a Tk shell (wish)
#
tkwait window .


