package require vtk

# Image pipeline

vtkPNGReader reader
reader SetFileName $VTK_DATA_ROOT/Data/fullhead15.png

vtkImageClip clip
clip SetInput [reader GetOutput]
clip SetOutputWholeExtent 80 230 80 230 0 0
clip ClipDataOff

vtkImageGradientMagnitude gradient
gradient SetDimensionality 2
gradient SetInput [clip GetOutput]
gradient HandleBoundariesOff



vtkImageChangeInformation slide
slide SetInput [gradient GetOutput]
slide SetExtentTranslation -100 -100 0

vtkImageViewer viewer
viewer DebugOn
viewer SetInput [slide GetOutput]
viewer SetColorWindow -1000
viewer SetColorLevel 500

viewer Render

source [file join [file dirname [info script]] WindowLevelInterface.tcl]
