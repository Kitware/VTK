catch {load vtktcl}
source vtkImageInclude.tcl

# Image pipeline

vtkImageReader reader
reader ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff
reader DebugOn

vtkImageFlip flip
flip SetInput [reader GetOutput]
flip SetFilteredAxes $VTK_IMAGE_X_AXIS 
#flip BypassOn
#flip PreserveImageExtentOn

vtkImageViewer viewer
viewer SetInput [flip GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 2000
viewer SetColorLevel 1000

#make interface
source WindowLevelInterface.tcl







