package require vtk

# Image pipeline

vtkImageReader2Factory createReader
set reader [createReader CreateImageReader2 "$VTK_DATA_ROOT/Data/t3_grid_0.mnc"]
$reader SetFileName "$VTK_DATA_ROOT/Data/t3_grid_0.mnc"


vtkImageViewer viewer
viewer SetInputConnection [$reader GetOutputPort]
viewer SetColorWindow 65535
viewer SetColorLevel 0


#make interface
viewer Render
$reader UnRegister viewer







