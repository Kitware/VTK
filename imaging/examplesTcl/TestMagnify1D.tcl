catch {load vtktcl}
# Doubles the The number of images (z dimension).

source vtkImageInclude.tcl

# Image pipeline

vtkImageReader reader
#reader DebugOn
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff

vtkImageMagnify1D magnify
magnify SetFilteredAxis $VTK_IMAGE_Y_AXIS
magnify SetInput [reader GetOutput]
magnify SetMagnificationFactor 2
magnify InterpolateOn
magnify ReleaseDataFlagOff

vtkImageViewer viewer
#viewer DebugOn
viewer SetInput [magnify GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 2000
viewer SetColorLevel 1000


# make interface
source WindowLevelInterface.tcl





