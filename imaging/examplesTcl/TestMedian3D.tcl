catch {load vtktcl}
# Test the median filter.


source vtkImageInclude.tcl




# Image pipeline

vtkImageReader reader
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff
#reader DebugOn

vtkImageMedian3D median
median SetInput [reader GetOutput]
median SetKernelSize 7 7 1
median ReleaseDataFlagOff


vtkImageViewer viewer
#viewer DebugOn
viewer SetInput [median GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 2000
viewer SetColorLevel 1000

# make interface
source WindowLevelInterface.tcl








