package require vtk

# This scripts shows a compressed spectrum of an image.



# Image pipeline

vtkMetaImageReader reader
reader SetFileName "$VTK_DATA_ROOT/Data/foot/foot.mha"

vtkImageViewer2 viewer
viewer SetInputConnection [reader GetOutputPort]
viewer SetColorWindow 255
viewer SetColorLevel 127.5

vtkRenderWindowInteractor viewInt
viewer SetupInteractor viewInt
viewer Render








