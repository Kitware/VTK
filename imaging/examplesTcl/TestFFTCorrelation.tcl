# Performs a correlation in frequency domain.

catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }


source vtkImageInclude.tcl

vtkImageCanvasSource2D s1
s1 SetScalarType $VTK_FLOAT
s1 SetExtent 0 255 0 255 0 0
s1 SetDrawColor 0
s1 FillBox 0 255 0 255
s1 SetDrawColor 2.0
s1 FillTriangle 10 100  190 150  40 250

vtkImageCanvasSource2D s2
s2 SetScalarType $VTK_FLOAT
s2 SetExtent 0 31 0 31 0 0
s2 SetDrawColor 0.0
s2 FillBox 0 31 0 31
s2 SetDrawColor 2.0
s2 FillTriangle 10 1  25 10  1 5


vtkImageFFT fft1
fft1 SetFilteredAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS
fft1 SetInput [s1 GetOutput]
fft1 ReleaseDataFlagOff


# Pad kernel out to same size as image.

vtkImageConstantPad pad2
pad2 SetInput [s2 GetOutput]
pad2 SetOutputWholeExtent 0 255 0 255 0 0

vtkImageFFT fft2
fft2 SetFilteredAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS
fft2 SetInput [pad2 GetOutput]
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
rfft SetInput [mult GetOutput]

vtkImageExtractComponents real
real SetInput [rfft GetOutput]
real SetComponents 0

vtkImageViewer viewer
viewer SetInput [real GetOutput]
viewer SetColorWindow 256
viewer SetColorLevel 127.5


# make interface
source WindowLevelInterface.tcl





