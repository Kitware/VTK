catch {load vtktcl}
# Halves the size of the image in the x, Y and Z dimensions.
# Computes the whole volume, but streams the input using the streaming
# functionality in vtkImageFilter class.


source vtkImageInclude.tcl


# Image pipeline

vtkImageReader reader
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff
[reader GetOutput] SetMemoryLimit 150

vtkImageShrink3D shrink
shrink SetInput [reader GetOutput]
shrink SetShrinkFactors 2 2 2
shrink AveragingOn
#shrink DebugOn

vtkImageViewer viewer
#viewer DebugOn
viewer SetInput [shrink GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 3000
viewer SetColorLevel 1500

#make interface
source WindowLevelInterface.tcl







