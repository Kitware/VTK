catch {load vtktcl}

source vtkImageInclude.tcl
source TkImageViewerInteractor.tcl


# This script uses a vtkTkRenderWidget to create a
# Tk widget that is associated with a vtkRenderWindow.
#source TkInteractor.tcl


# Image pipeline

vtkImageReader reader
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff
reader ReleaseDataFlagOff
#reader DebugOn

vtkImageResample magnify
magnify SetDimensionality 3
magnify SetInput [reader GetOutput]
magnify SetAxisOutputSpacing $VTK_IMAGE_X_AXIS 0.52
magnify SetAxisOutputSpacing $VTK_IMAGE_Y_AXIS 2.2
magnify SetAxisOutputSpacing $VTK_IMAGE_Z_AXIS 0.8
magnify ReleaseDataFlagOff
#magnify SetProgressMethod {set pro [magnify GetProgress]; puts "Completed $pro"; flush stdout}

vtkImageViewer viewer
viewer SetInput [magnify GetOutput]
viewer SetZSlice 30
viewer SetColorWindow 2000
viewer SetColorLevel 1000


proc Resize {w h} {
   global VTK_IMAGE_X_AXIS VTK_IMAGE_Y_AXIS

   magnify SetAxisOutputSpacing $VTK_IMAGE_X_AXIS [expr 256.0 / $w]
   magnify SetAxisOutputSpacing $VTK_IMAGE_Y_AXIS [expr 256.0 / $h]
}

# Create the GUI: two renderer widgets and a quit button
#
wm withdraw .
toplevel .top 

frame .top.f1 

vtkTkImageViewerWidget .top.f1.r1 -width 491 -height 116 -iv viewer
#    BindTkRenderWidget .top.f1.r1

button .top.btn  -text Quit -command exit
label .top.label -text "Resize this window"


pack .top.f1.r1 -side left -padx 3 -pady 3 -fill both -expand t
pack .top.f1  -fill both -expand t
pack .top.label -fill x
pack .top.btn -fill x


BindTkImageViewer .top.f1.r1 
bind .top.f1.r1 <Configure> {Resize %w %h}




