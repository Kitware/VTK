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
reader DebugOn
#reader Update


vtkImageMIPFilter mip
mip SetInput [reader GetOutput]
mip MIPZOn
mip SetProjectionRange 0 92

vtkImageViewer viewer
viewer SetInput [mip GetOutput]
viewer SetColorWindow 3000
viewer SetColorLevel 1500
viewer SetOriginLocationToUpperLeft
#viewer DebugOn
#viewer Render

#make interface
source WindowLevelInterface.tcl







