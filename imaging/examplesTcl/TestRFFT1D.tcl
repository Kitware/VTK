catch {load vtktcl}
# This scripts test the reverse Fourier transform by restoring an image.

source vtkImageInclude.tcl

# Image pipeline

vtkImageVolume16Reader reader
[reader GetCache] ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataDimensions 256 256 93
reader SetFilePrefix "../../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff
#reader DebugOn

vtkImageFFT1D fft
fft SetFilteredAxis $VTK_IMAGE_Y_AXIS
fft SetInput [reader GetOutput]

vtkImageRFFT1D rfft
rfft SetFilteredAxis $VTK_IMAGE_Y_AXIS
rfft SetInput [fft GetOutput]
rfft ReleaseDataFlagOff

vtkImageViewer viewer
viewer SetInput [rfft GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 2000
viewer SetColorLevel 1000
viewer ColorFlagOn
viewer SetRedComponent 0
viewer SetGreenComponent 1
viewer SetBlueComponent 1


source WindowLevelInterface.tcl








