catch {load vtktcl}
# Halves the size of the image in the x, Y and Z dimensions.


source vtkImageInclude.tcl


# Image pipeline

vtkImageReader reader
#reader DebugOn
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "../../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff

vtkImageSubsample3D shrink
shrink SetInput [reader GetOutput]
shrink SetShrinkFactors 2 2 2
shrink ReleaseDataFlagOff

vtkImageViewer viewer
#viewer DebugOn
viewer SetInput [shrink GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 2000
viewer SetColorLevel 1000

# make interface
source WindowLevelInterface.tcl


