package require vtk

# A script to test the threshold filter.
# Values above 2000 are set to 255.
# Values below 2000 are set to 0.


# Image pipeline

vtkPNGReader reader
reader SetFileName $VTK_DATA_ROOT/Data/fullhead15.png

vtkImageCast cast
cast SetOutputScalarTypeToShort
cast SetInput [reader GetOutput]

vtkImageThreshold thresh
thresh SetInput [cast GetOutput]
thresh ThresholdByUpper 2000.0
thresh SetInValue 0
thresh SetOutValue 200
thresh ReleaseDataFlagOff

vtkImageCityBlockDistance dist
dist SetDimensionality 2
dist SetInput [thresh GetOutput]

vtkImageViewer viewer
viewer SetInput [dist GetOutput]
viewer SetColorWindow 117
viewer SetColorLevel 43

viewer Render







