catch {load vtktcl}

source vtkImageInclude.tcl
source TkImageViewerInteractor.tcl


# This script tests the Append filter that puts two images together.

vtkPNMReader reader1
reader1 ReleaseDataFlagOff
reader1 SetFileName "../../../vtkdata/earth.ppm"

vtkPNMReader reader2
reader2 ReleaseDataFlagOff
reader2 SetFileName "../../../vtkdata/masonry.ppm"


vtkImageAppend append
append SetAppendAxis 0
append AddInput [reader1 GetOutput]
append AddInput [reader2 GetOutput]

# clip to make sure translation of extents is working correctly
vtkImageClip clip
clip SetInput [append GetOutput]
clip SetOutputWholeExtent 100 700 20 235 0 0
#clip SetOutputWholeExtent 0 767 20 230 0 0
clip ReleaseDataFlagOff


vtkImageViewer viewer
viewer SetInput [clip GetOutput]
viewer SetColorWindow 256
viewer SetColorLevel 127.5

# Create the GUI: two renderer widgets and a quit button
#
wm withdraw .
toplevel .top 

frame .top.f1 

vtkTkImageViewerWidget .top.f1.r1 -width 768 -height 256 -iv viewer
#    BindTkRenderWidget .top.f1.r1

button .top.btn  -text Quit -command exit

pack .top.f1.r1 -side left -padx 3 -pady 3 -fill both -expand t
pack .top.f1  -fill both -expand t
pack .top.btn -fill x


BindTkImageViewer .top.f1.r1 

