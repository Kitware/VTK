# Show the constant kernel.  Smooth an impulse function.

catch {load vtktcl}

source vtkImageInclude.tcl

vtkImageReader reader
reader ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 1
reader SetFilePrefix "../../../vtkdata/heart"
reader SetDataByteOrderToBigEndian
reader SetOutputScalarTypeToFloat

vtkImageGaussianSmooth smooth
smooth SetFilteredAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS
smooth SetInput [reader GetOutput]
smooth SetStandardDeviations 4.0 4.0
smooth SetRadiusFactors 2.0 2.0




vtkImageCanvasSource2D canvas
canvas SetScalarType $VTK_FLOAT
canvas SetExtent -10 10 -10 10
# back ground zero
canvas SetDrawColor 0
canvas FillBox -10 10 -10 10
# impulse
canvas SetDrawColor 8000
canvas DrawPoint 0 0
 
vtkImageGaussianSmooth smooth2
smooth2 SetFilteredAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS
smooth2 SetInput [canvas GetOutput]
smooth2 SetStandardDeviations 4.0 4.0
smooth2 SetRadiusFactors 3.0 3.0
 
vtkImageMagnify magnify
magnify SetFilteredAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS
magnify InterpolateOff
magnify SetMagnificationFactors 5 5
magnify SetInput [smooth2 GetOutput]
 
vtkImageViewer viewer2
viewer2 SetInput [magnify GetOutput]
viewer2 SetColorWindow 99
viewer2 SetColorLevel 32


vtkImageViewer viewer1
viewer1 SetInput [reader GetOutput]
viewer1 SetZSlice 0
viewer1 SetColorWindow 400
viewer1 SetColorLevel 200

vtkImageViewer viewer3
viewer3 SetInput [smooth GetOutput]
viewer3 SetZSlice 0
viewer3 SetColorWindow 400
viewer3 SetColorLevel 200

# Create the GUI
wm withdraw .
toplevel .top 

frame .top.f1 

vtkTkImageViewerWidget .top.f1.r1 -width 256 -height 256 -iv viewer1
vtkTkImageViewerWidget .top.f1.r2 -width 105 -height 105 -iv viewer2
vtkTkImageViewerWidget .top.f1.r3 -width 256 -height 256 -iv viewer3

button .top.btn  -text Quit -command exit

pack .top.f1.r1 .top.f1.r2 .top.f1.r3 \
  -side left -padx 3 -pady 3 -expand t
pack .top.f1  -fill both -expand t
pack .top.btn -fill x





