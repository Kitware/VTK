package require vtk

# Image pipeline

vtkPNGReader reader
reader SetFileName "$VTK_DATA_ROOT/Data/fullhead15.png"

# Take the gradient in X, and smooth in Y

# Create a simple gradient filter
vtkFloatArray kernel
kernel SetNumberOfTuples 3
kernel InsertValue 0 -1
kernel InsertValue 1  0
kernel InsertValue 2  1

# Create a gaussian for Y
set sigma 1.5
set sigma2 [expr $sigma * $sigma]
vtkFloatArray gaussian
gaussian SetNumberOfTuples 31
for { set i 0 } { $i < 31 } { incr i } {
  set x [expr $i - 15]
  set g [expr exp ( - ( $x * $x ) / (2.0 * $sigma2) ) / ( sqrt ( 2.0 * 3.1415 ) * $sigma ) ]
  gaussian InsertValue $i $g
}

vtkImageSeparableConvolution convolve
convolve SetInputConnection [reader GetOutputPort]
convolve SetDimensionality 2
convolve SetXKernel kernel
convolve SetYKernel gaussian

vtkImageViewer viewer
#viewer DebugOn
viewer SetInputConnection [convolve GetOutputPort]
viewer SetColorWindow 500
viewer SetColorLevel 100

wm withdraw .

viewer Render
