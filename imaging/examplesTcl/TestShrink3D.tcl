# Halves the size of the image in the x, Y and Z dimensions.

catch {load vtktcl}
source vtkImageInclude.tcl


# Image pipeline

vtkImageVolume16Reader reader
#reader DebugOn
reader SetDataByteOrderToLittleEndian
reader SetDataDimensions 256 256 93
reader SetFilePrefix "../../../data/fullHead/headsq"
reader SetDataMask 0x7fff

vtkImageShrink3D shrink
shrink SetInput [reader GetOutput]
shrink SetShrinkFactors 2 2 2
shrink ReleaseDataFlagOff

vtkImageViewer viewer
#viewer DebugOn
viewer SetInput [shrink GetOutput]
viewer SetZSlice 11
viewer SetColorWindow 2000
viewer SetColorLevel 1000
viewer Render


# make interface
source WindowLevelInterface.tcl







