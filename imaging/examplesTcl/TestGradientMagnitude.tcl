catch {load vtktcl}
source vtkImageInclude.tcl


# Image pipeline

vtkImageReader reader
#reader DebugOn
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff

vtkImageGradientMagnitude gradient
gradient SetDimensionality 3
gradient SetInput [reader GetOutput]

vtkImageViewer viewer
#viewer DebugOn
viewer SetInput [gradient GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 1000
viewer SetColorLevel 500


# make interface
source WindowLevelInterface.tcl
