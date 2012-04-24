package require vtk



# Image pipeline

vtkPNGReader reader
reader SetFileName "$VTK_DATA_ROOT/Data/fullhead15.png"

vtkSimpleImageFilterExample gradient
gradient SetInputConnection [reader GetOutputPort]

vtkImageViewer viewer
#viewer DebugOn
viewer SetInputConnection [gradient GetOutputPort]
viewer SetColorWindow 1000
viewer SetColorLevel 500


viewer Render
