catch {load vtktcl}
# This script shows the result of an ideal lowpass filter in  spatial domain

source vtkImageInclude.tcl

# Image pipeline

vtkImageVolume16Reader reader
reader SetDataByteOrderToLittleEndian
reader SetDataDimensions 256 256 93
reader SetFilePrefix "../../../data/fullHead/headsq"
reader SetDataMask 0x7fff
#reader DebugOn

vtkImageFFT fft
fft SetFilteredAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS
fft SetInput [reader GetOutput]
#fft DebugOn

vtkImageFourierIdealLowPass lowPass
lowPass SetInput [fft GetOutput]
lowPass SetXCutOff 0.04
lowPass SetYCutOff 0.04
lowPass ReleaseDataFlagOff
#lowPass DebugOn

vtkImageRFFT rfft
rfft SetFilteredAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS
rfft SetInput [lowPass GetOutput]
#fft DebugOn

vtkImageViewer viewer
viewer SetInput [rfft GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 2000
viewer SetColorLevel 1000
#viewer DebugOn


# make interface
source WindowLevelInterface.tcl







