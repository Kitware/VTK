package require vtk

# This script shows how to use vtkImageEuclideanDistance

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

vtkImageEuclideanDistance dist
dist SetInput [thresh GetOutput]
dist SetAlgorithmToSaitoCached

vtkImageViewer viewer
viewer SetInput [dist GetOutput]
viewer SetColorWindow 117
viewer SetColorLevel 43

viewer Render

source [file join [file dirname [info script]] WindowLevelInterface.tcl]
