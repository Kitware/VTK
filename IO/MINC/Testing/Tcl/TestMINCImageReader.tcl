package require vtk

# Image pipeline

vtkMINCImageReader reader
reader SetFileName "$VTK_DATA_ROOT/Data/t3_grid_0.mnc"


vtkImageViewer viewer
viewer SetInputConnection [reader GetOutputPort]
viewer SetColorWindow 65535
viewer SetColorLevel 0


#make interface
viewer Render






