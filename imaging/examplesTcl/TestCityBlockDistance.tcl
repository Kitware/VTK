catch {load vtktcl}
# A script to test the threshold filter.
# Values above 2000 are set to 255.
# Values below 2000 are set to 0.

source vtkImageInclude.tcl

# Image pipeline

vtkImageReader reader
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff

vtkImageCast cast
cast SetOutputScalarType $VTK_SHORT
cast SetInput [reader GetOutput]

vtkImageThreshold thresh
thresh SetInput [cast GetOutput]
thresh ThresholdByUpper 2000.0
thresh SetInValue 0
thresh SetOutValue 200
thresh ReleaseDataFlagOff

vtkImageCityBlockDistance dist
dist SetDimensionality 2
dist SetInput [thresh GetOutput]

vtkImageViewer viewer
viewer SetInput [dist GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 117
viewer SetColorLevel 43

# make interface
source WindowLevelInterface.tcl







