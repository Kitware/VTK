catch {load vtktcl}
source vtkImageInclude.tcl

# Image pipeline

vtkImageVolume16Reader reader
reader ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataDimensions 256 256 93
reader SetFilePrefix "../../../data/fullHead/headsq"
reader SetDataMask 0x7fff
#reader DebugOn

vtkImageRange3D range
range SetInput [reader GetOutput]
range SetKernelSize 5 5 5

vtkImageViewer viewer
viewer SetInput [range GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 1000
viewer SetColorLevel 500
#viewer DebugOn


source WindowLevelInterface.tcl


