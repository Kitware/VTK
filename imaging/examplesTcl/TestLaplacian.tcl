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

vtkImageLaplacian laplacian
laplacian SetDimensionality 3
laplacian SetInput [reader GetOutput]

vtkImageViewer viewer
viewer SetInput [laplacian GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 2000
viewer SetColorLevel 0
#viewer DebugOn


source WindowLevelInterface.tcl


