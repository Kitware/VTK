package require vtk

# Image pipeline

vtkImageReader2Factory createReader
set reader [createReader CreateImageReader2 "$VTK_DATA_ROOT/Data/libtif/test.tif"]
$reader SetFileName "$VTK_DATA_ROOT/Data/libtiff/test.tif"

vtkImageViewer viewer
viewer SetInput [$reader GetOutput]
viewer SetColorWindow 256
viewer SetColorLevel 127.5


#make interface
viewer Render