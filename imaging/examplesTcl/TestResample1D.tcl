catch {load vtktcl}
# Doubles the The number of images (x dimension).


source vtkImageInclude.tcl


# Image pipeline

vtkImageVolume16Reader reader
reader SetDataByteOrderToLittleEndian
reader SetDataDimensions 256 256 93
reader SetFilePrefix "../../../data/fullHead/headsq"
reader SetDataMask 0x7fff
#reader DebugOn

vtkImageResample1D magnify
magnify SetInput [reader GetOutput]
magnify SetFilteredAxis $VTK_IMAGE_Y_AXIS
magnify SetOutputSpacing 1.2
magnify ReleaseDataFlagOff

vtkImageViewer viewer
#viewer DebugOn
viewer SetInput [magnify GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 2000
viewer SetColorLevel 1000
viewer SetOriginLocationToUpperLeft


# make interface
source WindowLevelInterface.tcl




