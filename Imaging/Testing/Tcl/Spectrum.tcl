package require vtk

# This scripts shows a compressed spectrum of an image.



# Image pipeline

vtkPNGReader reader
reader SetFileName $VTK_DATA_ROOT/Data/fullhead15.png

vtkImageFFT fft
fft SetInput [reader GetOutput]
fft ReleaseDataFlagOff
#fft DebugOn

vtkImageMagnitude magnitude
magnitude SetInput [fft GetOutput]
magnitude ReleaseDataFlagOff

vtkImageFourierCenter center
center SetInput [magnitude GetOutput]

vtkImageLogarithmicScale compress
compress SetInput [center GetOutput]
compress SetConstant 15

vtkImageViewer2 viewer
viewer SetInput [compress GetOutput]
viewer SetColorWindow 150
viewer SetColorLevel 170

vtkRenderWindowInteractor viewInt
viewer SetupInteractor viewInt
viewer Render








