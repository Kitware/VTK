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
magnify SetDimensionality 3
magnify SetInput [reader GetOutput]
magnify SetAxisOutputSpacing $VTK_IMAGE_X_AXIS 0.3
magnify SetAxisOutputSpacing $VTK_IMAGE_Y_AXIS 2.2
magnify SetAxisOutputSpacing $VTK_IMAGE_Z_AXIS 0.8
magnify ReleaseDataFlagOff


vtkImageViewer viewer
viewer SetInput [magnify GetOutput]
viewer SetZSlice 30
viewer SetColorWindow 2000
viewer SetColorLevel 1000


# make interface
source WindowLevelInterface.tcl




