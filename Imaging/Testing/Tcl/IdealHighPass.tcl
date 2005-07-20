package require vtk

# This script shows the result of an ideal highpass filter in  spatial domain


# Image pipeline
vtkImageReader2Factory createReader
set reader [createReader CreateImageReader2 "$VTK_DATA_ROOT/Data/fullhead15.png"]
$reader SetFileName "$VTK_DATA_ROOT/Data/fullhead15.png"

vtkImageFFT fft
fft SetInputConnection [$reader GetOutputPort]

vtkImageIdealHighPass highPass
highPass SetInputConnection [fft GetOutputPort]
highPass SetXCutOff 0.1
highPass SetYCutOff 0.1
highPass ReleaseDataFlagOff

vtkImageRFFT rfft
rfft SetInputConnection [highPass GetOutputPort]

vtkImageExtractComponents real
real SetInputConnection [rfft GetOutputPort]
real SetComponents 0


vtkImageViewer viewer
viewer SetInputConnection [real GetOutputPort]
viewer SetColorWindow 500
viewer SetColorLevel 0


viewer Render
$reader UnRegister viewer







