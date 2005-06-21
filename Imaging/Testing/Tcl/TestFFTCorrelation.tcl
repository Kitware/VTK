package require vtk
package require vtkinteraction

# Performs a correlation in frequency domain.

vtkImageCanvasSource2D s1
s1 SetScalarTypeToFloat
s1 SetExtent 0 255 0 255 0 0
s1 SetDrawColor 0
s1 FillBox 0 255 0 255
s1 SetDrawColor 2.0
s1 FillTriangle 10 100  190 150  40 250

vtkImageCanvasSource2D s2
s2 SetScalarTypeToFloat
s2 SetExtent 0 31 0 31 0 0
s2 SetDrawColor 0.0
s2 FillBox 0 31 0 31
s2 SetDrawColor 2.0
s2 FillTriangle 10 1  25 10  1 5


vtkImageFFT fft1
fft1 SetDimensionality 2
fft1 SetInputConnection [s1 GetOutputPort]
fft1 ReleaseDataFlagOff


# Pad kernel out to same size as image.

vtkImageConstantPad pad2
pad2 SetInputConnection [s2 GetOutputPort]
pad2 SetOutputWholeExtent 0 255 0 255 0 0

vtkImageFFT fft2
fft2 SetDimensionality 2
fft2 SetInputConnection [pad2 GetOutputPort]
fft2 ReleaseDataFlagOff

# conjugate is necessary for correlation (not convolution)
vtkImageMathematics conj
conj SetOperationToConjugate
conj SetInput1 [fft2 GetOutput]

# Corrleation is multiplication in frequencey space.
vtkImageMathematics mult
mult SetOperationToComplexMultiply
mult SetInput1 [fft1 GetOutput]
mult SetInput2 [conj GetOutput]

vtkImageRFFT rfft
rfft SetDimensionality 2
rfft SetInputConnection [mult GetOutputPort]

vtkImageExtractComponents real
real SetInputConnection [rfft GetOutputPort]
real SetComponents 0

vtkImageViewer viewer
viewer SetInputConnection [real GetOutputPort]
viewer SetColorWindow 256
viewer SetColorLevel 127.5


# make interface
source [file join [file dirname [info script]] WindowLevelInterface.tcl]
