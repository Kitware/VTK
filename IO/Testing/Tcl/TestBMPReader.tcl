# this test is designed to check the operation of the 8bit
# export of BMPs

package require vtk

# Image pipeline

vtkBMPReader reader
reader SetFileName "$VTK_DATA_ROOT/Data/masonry.bmp"
reader SetAllow8BitBMP 1

vtkImageMapToColors map
map SetInput [reader GetOutput]
map SetLookupTable [reader GetLookupTable]
map SetOutputFormatToRGB

vtkImageViewer viewer
viewer SetInput [map GetOutput]
viewer SetColorWindow 256
viewer SetColorLevel 127.5

#make interface
viewer Render







