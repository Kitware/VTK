catch {load vtktcl}
# This scripts the reverse FFT. Pipeline is Reader->FFT->RFFT->Viewer.
# Output should be the same as Reader.


source vtkImageInclude.tcl

# Image pipeline

vtkImageVolume16Reader reader
reader SetDataByteOrderToLittleEndian
reader SetDataDimensions 256 256 93
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff
#reader DebugOn

vtkImageFFT fft
fft SetFilteredAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS
fft SetInput [reader GetOutput]
#fft DebugOn

vtkImageRFFT rfft
rfft SetFilteredAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS
rfft SetInput [fft GetOutput]
rfft ReleaseDataFlagOff
#fft DebugOn


vtkImageViewer viewer
viewer SetInput [rfft GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 2000
viewer SetColorLevel 1000
#viewer DebugOn


# make interface
source WindowLevelInterface.tcl










