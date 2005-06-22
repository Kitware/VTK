package require vtk

# Image pipeline

vtkImageReader2Factory createReader
set reader [createReader CreateImageReader2 "$VTK_DATA_ROOT/Data/beach.jpg"]
$reader SetFileName "$VTK_DATA_ROOT/Data/beach.jpg"

vtkImageViewer viewer
viewer SetInputConnection [$reader GetOutputPort]
viewer SetColorWindow 256
viewer SetColorLevel 127.5


#make interface
viewer Render
$reader UnRegister viewer







