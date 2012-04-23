package require vtk



# Image pipeline

vtkGESignaReader reader
reader SetFileName "$VTK_DATA_ROOT/Data/E07733S002I009.MR"

vtkImageGradientMagnitude gradient
gradient SetDimensionality 2
gradient SetInputConnection [reader GetOutputPort]

vtkImageViewer viewer
viewer SetInputConnection [gradient GetOutputPort]
viewer SetColorWindow 250
viewer SetColorLevel 125


viewer Render
