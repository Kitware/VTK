catch {load vtktcl}
# This script is for testing the 1d Gaussian Smooth filter.

source vtkImageInclude.tcl

set sliceNumber 22


# Image pipeline

vtkImageReader reader
#reader DebugOn
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff

vtkImageGaussianSmooth1D smooth
smooth SetInput [reader GetOutput]
smooth SetStride 2
smooth SetStandardDeviation 6
smooth SetFilteredAxis $VTK_IMAGE_Y_AXIS
smooth SetRadiusFactor 1.5
smooth ReleaseDataFlagOff


vtkImageViewer viewer
#viewer DebugOn
viewer SetInput [smooth GetOutput]
viewer SetZSlice $sliceNumber
viewer SetColorWindow 1200
viewer SetColorLevel 600


#make interface
source WindowLevelInterface.tcl
