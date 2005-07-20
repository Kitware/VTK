package require vtk

# This scripts shows a compressed spectrum of an image.



# Image pipeline

vtkPNGReader reader
reader SetFileName "$VTK_DATA_ROOT/Data/fullhead15.png"

vtkImageFFT fft
fft SetInputConnection [reader GetOutputPort]
fft ReleaseDataFlagOff
#fft DebugOn

vtkImageMagnitude magnitude
magnitude SetInputConnection [fft GetOutputPort]
magnitude ReleaseDataFlagOff

vtkImageFourierCenter center
center SetInputConnection [magnitude GetOutputPort]

vtkImageLogarithmicScale compress
compress SetInputConnection [center GetOutputPort]
compress SetConstant 15

vtkImageViewer2 viewer
viewer SetInputConnection [compress GetOutputPort]
viewer SetColorWindow 150
viewer SetColorLevel 170

vtkRenderWindowInteractor viewInt
viewer SetupInteractor viewInt
viewer Render








