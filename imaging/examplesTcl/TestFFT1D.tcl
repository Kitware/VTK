catch {load vtktcl}
# This scripts shows the real component of an image in frequencey space.

source vtkImageInclude.tcl

# Image pipeline

vtkImageReader reader
[reader GetCache] ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff
#reader DebugOn

vtkImageFFT1D fft
fft SetFilteredAxis $VTK_IMAGE_Y_AXIS
fft SetInput [reader GetOutput]
fft ReleaseDataFlagOff

vtkImageViewer viewer
viewer SetInput [fft GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 8000
viewer SetColorLevel 2500


source WindowLevelInterface.tcl








