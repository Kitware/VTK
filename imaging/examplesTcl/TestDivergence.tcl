catch {load vtktcl}
# Divergence measures rate of change of gradient.

source vtkImageInclude.tcl

# Image pipeline

vtkImageReader reader
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff
#reader DebugOn

vtkImageGradient gradient
gradient SetDimensionality 3
gradient SetInput [reader GetOutput]

vtkImageDivergence derivative
derivative SetDimensionality 3
derivative SetInput [gradient GetOutput]

vtkImageViewer viewer
viewer SetInput [derivative GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 1000
viewer SetColorLevel 0


# make interface
source WindowLevelInterface.tcl







