package require vtktcl

# A script to test DilationErode filter
# First the image is thresholded.
# It is the dilated with a spher of radius 5.


# Image pipeline

vtkImageReader reader
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 63 0 63 1 93
reader SetFilePrefix "$VTK_DATA_ROOT/Data/headsq/quarter"
reader SetDataMask 0x7fff

vtkImageThreshold thresh
thresh SetInput [reader GetOutput]
thresh ThresholdByUpper 2000.0
thresh SetInValue 255
thresh SetOutValue 0

vtkImageDilateErode3D dilate
dilate SetInput [thresh GetOutput]
dilate SetDilateValue 255
dilate SetErodeValue 0
dilate SetKernelSize 5 5 5
dilate ReleaseDataFlagOff

vtkImageViewer viewer
viewer SetInput [dilate GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 255
viewer SetColorLevel 127.5
#viewer DebugOn


viewer Render
