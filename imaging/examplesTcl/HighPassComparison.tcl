catch {load vtktcl}

source vtkImageInclude.tcl
source TkImageViewerInteractor.tcl


vtkImageReader reader
reader ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 94
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff


vtkImageCast cast
cast SetInput [reader GetOutput]
cast SetOutputScalarTypeToFloat

vtkImageFFT fft
fft SetFilteredAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS
fft SetInput [cast GetOutput]
fft ReleaseDataFlagOff

vtkImageIdealHighPass highPass1
highPass1 SetInput [fft GetOutput]
highPass1 SetXCutOff 0.15
highPass1 SetYCutOff 0.15
highPass1 ReleaseDataFlagOff

vtkImageButterworthHighPass highPass2
highPass2 SetInput [fft GetOutput]
highPass2 SetOrder 2
highPass2 SetXCutOff 0.15
highPass2 SetYCutOff 0.15
highPass2 ReleaseDataFlagOff

vtkImageRFFT rfft1
rfft1 SetFilteredAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS
rfft1 SetInput [highPass1 GetOutput]

vtkImageRFFT rfft2
rfft2 SetFilteredAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS
rfft2 SetInput [highPass2 GetOutput]

vtkImageExtractComponents real1
real1 SetInput [rfft1 GetOutput]
real1 SetComponents 0

vtkImageExtractComponents real2
real2 SetInput [rfft2 GetOutput]
real2 SetComponents 0

vtkImageViewer viewer1
viewer1 SetInput [highPass1 GetOutput]
viewer1 SetZSlice 22
viewer1 SetColorWindow 10000
viewer1 SetColorLevel 5000

vtkImageViewer viewer2
viewer2 SetInput [highPass2 GetOutput]
viewer2 SetZSlice 22
viewer2 SetColorWindow 10000
viewer2 SetColorLevel 5000

vtkImageViewer viewer3
viewer3 SetInput [real1 GetOutput]
viewer3 SetZSlice 22
viewer3 SetColorWindow 500
viewer3 SetColorLevel 0

vtkImageViewer viewer
viewer SetInput [real2 GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 500
viewer SetColorLevel 0

# Create the GUI
wm withdraw .
toplevel .top 

frame .top.f1 
frame .top.f2

vtkTkImageViewerWidget .top.f1.r1 -width 256 -height 256 -iv viewer1
vtkTkImageViewerWidget .top.f1.r2 -width 256 -height 256 -iv viewer2
vtkTkImageViewerWidget .top.f2.r3 -width 256 -height 256 -iv viewer3
vtkTkImageViewerWidget .top.f2.r4 -width 256 -height 256 -iv viewer

button .top.btn  -text Quit -command exit

pack .top.f1.r1 .top.f1.r2 \
  -side left -padx 3 -pady 3 -expand t
pack .top.f2.r3 .top.f2.r4 \
  -side left -padx 3 -pady 3 -expand t
pack .top.f1 .top.f2  -fill both -expand t
pack .top.btn -fill x


BindTkImageViewer .top.f1.r1 
BindTkImageViewer .top.f1.r2
BindTkImageViewer .top.f2.r3 
BindTkImageViewer .top.f2.r4 








