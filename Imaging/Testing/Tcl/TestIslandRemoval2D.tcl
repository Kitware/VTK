package require vtk

# A script to test the island removal filter.
# first the image is thresholded, then small islands are removed.


# Image pipeline

vtkPNGReader reader
reader SetFileName $VTK_DATA_ROOT/Data/fullhead15.png

vtkImageThreshold thresh
thresh SetInput [reader GetOutput]
thresh ThresholdByUpper 2000.0
thresh SetInValue 255
thresh SetOutValue 0
thresh ReleaseDataFlagOff

vtkImageIslandRemoval2D island
island SetInput [thresh GetOutput]
island SetIslandValue 255
island SetReplaceValue 128
island SetAreaThreshold 100
island SquareNeighborhoodOn

vtkImageViewer viewer
viewer SetInput [island GetOutput]
viewer SetColorWindow 255
viewer SetColorLevel 127.5
#viewer DebugOn

viewer Render







