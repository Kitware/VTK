# Divergence measures rate of change of gradient.
package require vtk


# Image pipeline

vtkPNGReader reader
reader SetFileName $VTK_DATA_ROOT/Data/fullhead15.png

vtkImageGradient gradient
gradient SetDimensionality 2
gradient SetInput [reader GetOutput]

vtkImageDivergence derivative
derivative SetInput [gradient GetOutput]

vtkImageViewer viewer
viewer SetInput [derivative GetOutput]
viewer SetColorWindow 1000
viewer SetColorLevel 0


viewer Render







