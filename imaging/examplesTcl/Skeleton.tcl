catch {load vtktcl}
# A script to test the threshold filter.
# Values above 2000 are set to 255.
# Values below 2000 are set to 0.

source vtkImageInclude.tcl
source TkImageViewerInteractor.tcl


# Image pipeline

vtkImageReader reader
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff
#reader DebugOn

vtkImageThreshold thresh
thresh SetInput [reader GetOutput]
thresh SetOutputScalarTypeToUnsignedChar
thresh ThresholdByUpper 1500.0
thresh SetInValue 255
thresh SetOutValue 0
thresh ReleaseDataFlagOff

vtkImageOpenClose3D my_close
my_close SetInput [thresh GetOutput]
my_close SetOpenValue 0
my_close SetCloseValue 255
my_close SetKernelSize 5 5 3
my_close ReleaseDataFlagOff


vtkImageSkeleton2D skeleton1
#skeleton1 BypassOn
skeleton1 SetInput [my_close GetOutput]
skeleton1 SetPrune 1
skeleton1 SetNumberOfIterations 20
skeleton1 ReleaseDataFlagOff

vtkImageViewer viewer1
viewer1 SetInput [my_close GetOutput]
viewer1 SetColorWindow 5
viewer1 SetColorLevel 1

vtkImageViewer viewer
#viewer SetInput [magnify GetOutput]
viewer SetInput [skeleton1  GetOutput]
viewer SetColorWindow 5
viewer SetColorLevel 1





# Create the GUI
wm withdraw .
toplevel .top 

frame .top.f1 
frame .top.f2

vtkTkImageViewerWidget .top.f1.r1 -width 256 -height 256 -iv viewer1
vtkTkImageViewerWidget .top.f1.r2 -width 256 -height 256 -iv viewer

button .top.btn  -text Quit -command exit

pack .top.f1.r1 .top.f1.r2 \
  -side left -padx 3 -pady 3 -expand t
pack .top.f1  -fill both -expand t
pack .top.btn -fill x


BindTkImageViewer .top.f1.r1 
BindTkImageViewer .top.f1.r2







