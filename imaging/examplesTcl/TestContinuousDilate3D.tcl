# Min of sphere around every pixel.

catch {load vtktcl}
source vtkImageInclude.tcl

# Image pipeline

vtkImageVolume16Reader reader
reader ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataDimensions 256 256 93
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff
#reader DebugOn

vtkImageContinuousDilate3D dilate
dilate SetInput [reader GetOutput]
dilate SetKernelSize 5 5 5

vtkImageViewer viewer
viewer SetInput [dilate GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 2000
viewer SetColorLevel 1000
#viewer DebugOn


source WindowLevelInterface.tcl


