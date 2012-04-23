# Divergence measures rate of change of gradient.
package require vtk


# Image pipeline

vtkPNGReader reader
reader SetFileName "$VTK_DATA_ROOT/Data/fullhead15.png"

vtkImageGradient gradient
gradient SetDimensionality 2
gradient SetInputConnection [reader GetOutputPort]

vtkImageDivergence derivative
derivative SetInputConnection [gradient GetOutputPort]

vtkImageViewer viewer
viewer SetInputConnection [derivative GetOutputPort]
viewer SetColorWindow 1000
viewer SetColorLevel 0


viewer Render







