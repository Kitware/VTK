catch {load vtktcl}
# This scripts the reverse FFT. Pipeline is Reader->FFT->RFFT->Viewer.
# Output should be the same as Reader.


source vtkImageInclude.tcl

# Image pipeline

vtkImageReader reader
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff
#reader DebugOn

vtkImageFFT fft
fft SetDimensionality 2
fft SetInput [reader GetOutput]
#fft DebugOn

vtkImageRFFT rfft
rfft SetDimensionality 2
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










