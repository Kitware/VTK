catch {load vtktcl}
# This script subtracts the 2D laplacian from an image to enhance the edges.


source vtkImageInclude.tcl
source TkImageViewerInteractor.tcl

# Image pipeline

vtkImageReader reader
#reader DebugOn
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff

vtkImageCast cast
cast SetInput [reader GetOutput]
cast SetOutputScalarTypeToFloat

vtkImageLaplacian lap
lap SetInput [cast GetOutput]
lap SetDimensionality 3

vtkImageMathematics subtract
subtract SetOperationToSubtract
subtract SetInput1 [cast GetOutput]
subtract SetInput2 [lap GetOutput]
subtract ReleaseDataFlagOff
#subtract BypassOn

vtkImageViewer viewer1
viewer1 SetInput [cast GetOutput]
viewer1 SetZSlice 22
viewer1 SetColorWindow 2000
viewer1 SetColorLevel 1000


vtkImageViewer viewer2
viewer2 SetInput [lap GetOutput]
viewer2 SetZSlice 22
viewer2 SetColorWindow 1000
viewer2 SetColorLevel 0


vtkImageViewer viewer
viewer SetInput [subtract GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 2000
viewer SetColorLevel 1000


# Create the GUI
wm withdraw .
toplevel .top 

frame .top.f1 

vtkTkImageViewerWidget .top.f1.r1 -width 256 -height 256 -iv viewer1
vtkTkImageViewerWidget .top.f1.r2 -width 256 -height 256 -iv viewer2
vtkTkImageViewerWidget .top.f1.r3 -width 256 -height 256 -iv viewer

button .top.btn  -text Quit -command exit

pack .top.f1.r1 .top.f1.r2 .top.f1.r3 \
  -side left -padx 3 -pady 3 -expand t
pack .top.f1 -fill both -expand t
pack .top.btn -fill x

BindTkImageViewer .top.f1.r1 
BindTkImageViewer .top.f1.r2
BindTkImageViewer .top.f1.r3 



