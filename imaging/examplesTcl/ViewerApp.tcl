#
# For the purposes of this exercise, you only need to make changes in
# four places.  These are labeled with "#1", "#2", "#3", and "#4".
#
# This exercise allows you to create a small image processing application.
# You can add filters to the pipeline and turn them on an off via a user
# interface.  
#
# Try adding a vtkImageGradient and a vtkImageMagnitude after the default
# vtkImageGaussianSmooth. Time permitting, you might want to try a FlipFilter
# of a PadFilter as well.
#
# Default event bindings:
#     Mouse motion - Probes the input data, displaying pixel position and value
#     Left Mouse Button - Window/Level (Contrast/Brightness)
#     Right Mouse Button - Changes slices when mouse moved up/down
#     Keypress "r" - Resets the window/level to a default setting
#

catch {load vtktcl}
source vtkImageInclude.tcl
source ViewerAppTkImageViewerInteractor.tcl


#
# Begin by setting up the Tk portion of the application
#

wm withdraw .
toplevel .top -visual best
wm title .top "Viz'99 VTK Imaging Exercise"

# menus
menu .top.menu -type menubar

menu .top.menu.file -tearoff 0
menu .top.menu.filters -title "Filters"
menu .top.menu.help -tearoff 0

.top.menu add cascade -menu .top.menu.file -label "File"
.top.menu add cascade -menu .top.menu.filters -label "Filters"
.top.menu add cascade -menu .top.menu.help -label "Help"

.top.menu.file add command -label "Quit" -command { exit }
.top.menu.help add command -label "User Interface" -command HelpUI

# Helper proc
proc AddFilterMenuItem { labelString command } {
    .top.menu.filters add checkbutton -label $labelString -variable $command -onvalue on -offvalue off -command "ActivateFilter $command"
}

#
# #1 -- To add a filter to the menu, call AddFilterMenuItem passing 
#       the "label" to use for the menu item and the name of your filter's 
#       instance. Example: Below is a filter called "gaussian" so we call 
#
#             AddFilterMenuItem Smoothing gaussian
#
AddFilterMenuItem Smoothing gaussian
AddFilterMenuItem "Edge Directions" gradient
AddFilterMenuItem Magnitude magnitude
AddFilterMenuItem "Flip Y" flipY
AddFilterMenuItem "Flip X" flipX

.top configure -menu .top.menu

# viewer frame
frame .top.f1
vtkImageViewer viewer
vtkTkImageViewerWidget .top.f1.v1 -width 512 -height 512 -iv viewer
pack .top.f1.v1 -padx 3 -pady 3 -side left -fill both -expand t

# annotation frames
frame .top.f2
frame .top.f2.f1
frame .top.f2.f2
frame .top.f2.f3
frame .top.f2.f4

label .top.f2.f1.label -relief sunken 
label .top.f2.f2.label -relief sunken 
label .top.f2.f3.label -relief sunken 
label .top.f2.f4.label -relief sunken 

pack .top.f2.f1.label -fill x 
pack .top.f2.f2.label -fill x 
pack .top.f2.f3.label -fill x 
pack .top.f2.f4.label -fill x 

pack .top.f2.f1 .top.f2.f2 .top.f2.f3 .top.f2.f4 -fill x -side left -expand t
pack .top.f1 -fill both -expand t
pack .top.f2 -fill x 


#
# Set up the vtk imaging pipeline
#

vtkImageReader reader
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff
#
# #2 -- Try uncommenting the following line and running the script.
#       How does the load time change?  How does the performance change
#       when all the filters are off and you adjust the "slice" that is 
#       displayed (right mouse button, drag up/down)?
#reader Update

vtkImageGaussianSmooth gaussian
gaussian SetInput [reader GetOutput]
gaussian SetStandardDeviations 2 2
gaussian BypassOn

#
# #3 -- Add additional filters here. Turn BypassOn on each to start.
#

vtkImageGradient gradient
gradient SetInput [gaussian GetOutput]
gradient BypassOn

vtkImageMagnitude magnitude
magnitude SetInput [gradient GetOutput]
magnitude BypassOn

vtkImageFlip flipY
flipY SetInput [magnitude GetOutput]
flipY SetFilteredAxis 1
flipY BypassOn

vtkImageFlip flipX
flipX SetInput [flipY GetOutput]
flipX SetFilteredAxis 0
flipX BypassOn

#
# #4 -- Set "lastfilter" to be the last filter in the pipeline
#
set lastFilter flipX

#
#
# You shouldn't need to change anything below this point.
#
#

#
# grab the viewer from the TkImageViewerWidget
#
set viewer [.top.f1.v1 GetImageViewer]
$viewer SetInput [$lastFilter GetOutput]
$viewer SetZSlice 14
ResetTkImageViewer .top.f1.v1

# resize the widget to fit the size of the data
set dims [[reader GetOutput] GetDimensions]
.top.f1.v1 configure -width [lindex $dims 0] -height [lindex $dims 1]

# make interface
BindTkImageViewer .top.f1.v1 

# tie labels to variables embedded in the Tk Widget
.top.f2.f1.label configure -textvariable [GetWidgetVariable .top.f1.v1 WindowLevelString]
.top.f2.f2.label configure -textvariable [GetWidgetVariable .top.f1.v1 PixelPositionString]
.top.f2.f3.label configure -textvariable [GetWidgetVariable .top.f1.v1 SliceString]


#
# Procs for toggling filters
# 
proc ActivateFilter { filtername } {
    global viewer
    upvar $filtername filtermode
    
    # make sure the filter exists
    set foo [catch {[info command $filtername] == $filtername}]
    if { $foo == 1 } {
  	# filter exists
  	if { $filtermode == "off" } {
  	    eval $filtername BypassOn
  	} else {
  	    eval $filtername BypassOff
  	}
	
	ResetTkImageViewer .top.f1.v1
    }
}

#
# Help windows
# 
proc HelpUI {} {

    if { [info commands .help] != ".help" } {

	toplevel .help
	wm title .help "User Interface Help"

	frame .help.f1
	label .help.f1.l0 -padx 3 -pady 3 -text "UI Bindings" -font bold
	label .help.f1.l1 -padx 3 -pady 3 -text \
	"
<Motion> - probe pixel
<B1> - Window/Level (Constrast/Brightness)
<B3> - Change slice
<KeyPress-r> - Reset window/level
	"
	pack .help.f1.l0 .help.f1.l1 -expand t -fill both
	pack .help.f1
    } else {
	raise .help
    }
}
