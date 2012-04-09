package require vtk

vtkBMPReader reader
reader SetFileName "$VTK_DATA_ROOT/Data/masonry.bmp"

# set the window/level
vtkImageViewer2 viewer
viewer SetInputConnection [reader GetOutputPort]
viewer SetColorWindow 100.0
viewer SetColorLevel 127.5

viewer Render
