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


vtkImageViewer viewer
viewer SetInput [reader GetOutput]
viewer SetZSlice 14
viewer SetDisplayExtent 0 255 0 255 0 20
viewer SetColorWindow 2000
viewer SetColorLevel 1000
#viewer DebugOn
viewer Render

viewer SetPosition 50 50

#make interface
#source WindowLevelInterface.tcl







