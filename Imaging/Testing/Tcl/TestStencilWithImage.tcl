package require vtk


# A script to test the stencil filter by using one image
# to stencil another


# Image pipeline

vtkBMPReader reader1
reader1 SetFileName "$VTK_DATA_ROOT/Data/masonry.bmp"

vtkPNMReader reader2
reader2 SetFileName "$VTK_DATA_ROOT/Data/B.pgm"

vtkImageTranslateExtent translate
translate SetInput [reader2 GetOutput]
translate SetTranslation 60 60 0

vtkImageToImageStencil imageToStencil
imageToStencil SetInput [translate GetOutput]
imageToStencil ThresholdBetween 0 127
# silly stuff to increase coverage
imageToStencil SetUpperThreshold [imageToStencil GetUpperThreshold]
imageToStencil SetLowerThreshold [imageToStencil GetLowerThreshold]

vtkImageStencil stencil
stencil SetInput [reader1 GetOutput]
stencil SetBackgroundValue 0
stencil ReverseStencilOn
stencil SetStencil [imageToStencil GetOutput]

vtkImageViewer viewer
viewer SetInput [stencil GetOutput]
viewer SetColorWindow 255.0
viewer SetColorLevel 127.5
viewer SetZSlice 0
viewer Render







