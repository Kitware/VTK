catch {load vtktcl}
# This script shows the magnitude of an image in frequency domain.


source vtkImageInclude.tcl


# Image pipeline

vtkImageReader reader
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 22 22
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff
#reader DebugOn

vtkImageCast cast
cast SetInput [reader GetOutput]
cast SetOutputScalarType $VTK_FLOAT

vtkImageShiftScale scale2
scale2 SetInput [cast GetOutput]
scale2 SetScale 0.05

vtkImageGradient gradient
gradient SetInput [scale2 GetOutput]
gradient SetDimensionality 3

vtkPNMReader pnm
pnm SetFileName "../../../vtkdata/masonry.ppm"

vtkImageCast cast2
cast2 SetInput [pnm GetOutput]
cast2 SetOutputScalarType $VTK_FLOAT

vtkImageDotProduct magnitude
magnitude SetInput1 [cast2 GetOutput]
magnitude SetInput2 [gradient GetOutput]

#vtkImageViewer viewer
vtkImageViewer viewer
viewer SetInput [magnitude GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 1000
viewer SetColorLevel 300
#viewer DebugOn


# make interface
source WindowLevelInterface.tcl





