catch {load vtktcl}
# Doubles the size of the image in the X and tripples in Y dimensions.
source vtkImageInclude.tcl





# Image pipeline

vtkImageVolume16Reader reader
#reader DebugOn
reader SetDataByteOrderToLittleEndian
reader SetDataDimensions 256 256 93
reader SetFilePrefix "../../../data/fullHead/headsq"
reader SetDataMask 0x7fff

vtkImageMagnify magnify
magnify SetInput [reader GetOutput]
magnify SetMagnificationFactors 3 2
magnify SetFilteredAxes $VTK_IMAGE_Y_AXIS $VTK_IMAGE_X_AXIS
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


