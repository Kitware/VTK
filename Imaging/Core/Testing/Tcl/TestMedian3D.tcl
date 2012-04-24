package require vtk

# Test the median filter.






# Image pipeline

vtkPNGReader reader
reader SetFileName "$VTK_DATA_ROOT/Data/fullhead15.png"

vtkImageMedian3D median
median SetInputConnection [reader GetOutputPort]
median SetKernelSize 7 7 1
median ReleaseDataFlagOff


vtkImageViewer viewer
#viewer DebugOn
viewer SetInputConnection [median GetOutputPort]
viewer SetColorWindow 2000
viewer SetColorLevel 1000

viewer Render








