package require vtk

vtkBMPReader reader
reader SetFileName $VTK_DATA_ROOT/Data/masonry.bmp

# set the window/level 
vtkImageViewer2 viewer
viewer SetInput [reader GetOutput]
viewer SetColorWindow 100.0
viewer SetColorLevel 127.5

viewer Render

#make interface
viewer Render

vtkWindowToImageFilter windowToimage
  windowToimage SetInput [viewer GetRenderWindow]


