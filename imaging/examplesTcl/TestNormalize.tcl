catch {load vtktcl}
# This script is for testing the normalize filter.

source vtkImageInclude.tcl
source TkImageViewerInteractor.tcl


# Image pipeline

vtkImageReader reader
#reader DebugOn
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff

vtkImageGradient gradient
gradient SetInput [reader GetOutput]
gradient SetDimensionality 3

vtkImageNormalize norm
norm SetInput [gradient GetOutput]

vtkImageViewer viewer
#viewer DebugOn
viewer SetInput [norm GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 1.0
viewer SetColorLevel 0.5




# Create the GUI: two renderer widgets and a quit button
#
wm withdraw .
toplevel .top 

frame .top.f1 

vtkTkImageViewerWidget .top.f1.r1 -width 256 -height 256 -iv viewer
#    BindTkRenderWidget .top.f1.r1

button .top.btn  -text Quit -command exit

pack .top.f1.r1 -side left -padx 3 -pady 3 -fill both -expand t
pack .top.f1  -fill both -expand t
pack .top.btn -fill x


BindTkImageViewer .top.f1.r1 







