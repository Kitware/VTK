package require vtk

# This script shows how to use vtkImageEuclideanDistance

# Image pipeline

vtkPNGReader reader
reader SetFileName "$VTK_DATA_ROOT/Data/fullhead15.png"

vtkImageCast cast
cast SetOutputScalarTypeToShort
cast SetInputConnection [reader GetOutputPort]

vtkImageThreshold thresh
thresh SetInputConnection [cast GetOutputPort]
thresh ThresholdByUpper 2000.0
thresh SetInValue 0
thresh SetOutValue 200
thresh ReleaseDataFlagOff

vtkImageEuclideanDistance dist
dist SetInputConnection [thresh GetOutputPort]
dist SetAlgorithmToSaito

vtkImageViewer viewer
viewer SetInputConnection [dist GetOutputPort]
viewer SetColorWindow 117
viewer SetColorLevel 43

viewer Render

source [file join [file dirname [info script]] WindowLevelInterface.tcl]
