package require vtk

# This script shows the result of an ideal lowpass filter in frequency space.


# Image pipeline

vtkPNGReader reader
reader SetFileName $VTK_DATA_ROOT/Data/fullhead15.png

vtkImageFFT fft
fft SetDimensionality 2
fft SetInput [reader GetOutput]
#fft DebugOn

vtkImageButterworthLowPass lowPass
lowPass SetInput [fft GetOutput]
lowPass SetOrder 2
lowPass SetXCutOff 0.2
lowPass SetYCutOff 0.1
lowPass ReleaseDataFlagOff
#lowPass DebugOn

vtkImageViewer viewer
viewer SetInput [lowPass GetOutput]
viewer SetColorWindow 10000
viewer SetColorLevel 5000
#viewer DebugOn


viewer Render


