package require vtk

# This script shows the result of an ideal highpass filter in frequency space.


# Image pipeline

vtkPNGReader reader
reader SetFileName "$VTK_DATA_ROOT/Data/fullhead15.png"

vtkImageFFT fft
fft SetDimensionality 2
fft SetInputConnection [reader GetOutputPort] 
#fft DebugOn

vtkImageButterworthHighPass highPass
highPass SetInputConnection [fft GetOutputPort]
highPass SetOrder 2
highPass SetXCutOff 0.2
highPass SetYCutOff 0.1
highPass ReleaseDataFlagOff
#highPass DebugOn

vtkImageViewer viewer
viewer SetInputConnection [highPass GetOutputPort]
viewer SetColorWindow 10000
viewer SetColorLevel 5000
#viewer DebugOn


viewer Render


