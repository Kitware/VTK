package require vtktcl

# Image pipeline

#reader DebugOn
vtkJPEGReader reader
reader SetFileName "$VTK_DATA_ROOT/Data/beach.jpg"

vtkImageViewer viewer
viewer SetInput [reader GetOutput]
viewer SetColorWindow 256
viewer SetColorLevel 127.5


#make interface
viewer Render







