catch {load vtktcl}
# Simple viewer for images.


source vtkImageInclude.tcl

# Image pipeline

vtkImageReader reader
reader ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff
#reader DebugOn
#reader Update

vtkImageClip clip
clip SetInput [reader GetOutput]
clip SetOutputWholeExtent 0 255 50 150 1 93
clip ReleaseDataFlagOff

vtkImageViewer viewer
viewer SetSize 256 256
viewer SetInput [clip GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 2000
viewer SetColorLevel 1000
#viewer DebugOn

#make interface
source WindowLevelInterface.tcl







