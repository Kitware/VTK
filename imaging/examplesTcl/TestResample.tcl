catch {load vtktcl}
# Doubles the The number of images (x dimension).


source vtkImageInclude.tcl


# Image pipeline

vtkImageReader reader
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff
reader ReleaseDataFlagOff
#reader DebugOn

vtkImageResample magnify
magnify SetInput [reader GetOutput]
magnify SetFilteredAxes $VTK_IMAGE_Z_AXIS $VTK_IMAGE_Y_AXIS
magnify SetAxisOutputSpacing $VTK_IMAGE_Z_AXIS 4.2
magnify SetAxisOutputSpacing $VTK_IMAGE_Y_AXIS 2.2
magnify ReleaseDataFlagOff

vtkImageViewer viewer
#viewer DebugOn
viewer SetInput [magnify GetOutput]
viewer SetZSlice 125
viewer SetColorWindow 2000
viewer SetColorLevel 1000


# make interface
source WindowLevelInterface.tcl




