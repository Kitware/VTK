catch {load vtktcl}
# This script shows the magnitude of an image in frequency domain.


source vtkImageInclude.tcl


# Image pipeline

vtkImageReader reader
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 0 92
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff

vtkImageGradient gradient
gradient SetInput [reader GetOutput]
gradient SetDimensionality 3
#gradient DebugOn

vtkImageMagnitude magnitude
magnitude SetInput [gradient GetOutput]

vtkImageViewer viewer
viewer SetInput [magnitude GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 1000
viewer SetColorLevel 200
#viewer DebugOn


#make interface
source WindowLevelInterface.tcl







