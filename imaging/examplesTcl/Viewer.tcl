catch {load vtktcl}
# Simple viewer for images.


source vtkImageInclude.tcl

# Image pipeline

vtkImageVolume16Reader reader
reader ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataDimensions 256 256 93
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff
#reader DebugOn
#reader Update


vtkImageViewer viewer
viewer SetInput [reader GetOutput]
viewer SetZSlice 22
viewer SetDisplayExtent 20 235 0 255
viewer SetColorWindow 2000
viewer SetColorLevel 1000
viewer SetOriginLocationToUpperLeft
#viewer DebugOn
#viewer Render

#make interface
source WindowLevelInterface.tcl







