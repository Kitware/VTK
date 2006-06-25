package require vtk

vtkGlobFileNames globFileNames
globFileNames AddFileNames "$VTK_DATA_ROOT/Data/headsq/quarter.*"

vtkSortSeriesFileNames sortFileNames
sortFileNames SetInputFileNames [globFileNames GetFileNames]
sortFileNames NumericSortOn

vtkImageReader2 reader
reader SetFileNames [sortFileNames GetFileNames]
reader SetDataExtent 0 63 0 63 1 1

# set Z slice to 2: if output is not numerically sorted, the wrong
# slice will be shown
vtkImageViewer viewer
viewer SetInputConnection [reader GetOutputPort]
viewer SetZSlice 2
viewer SetColorWindow 2000
viewer SetColorLevel 1000

viewer Render
reader Delete

