# This Script test the euclidean to polar by coverting 2D vectors 
# from a gradient into polar, which is converted into HSV, and then to RGB.
catch {load vtktcl}

source vtkImageInclude.tcl


# Image pipeline

vtkImageGaussianSource gauss
gauss SetWholeExtent 0 255 0 255 0 44 0 0
gauss SetCenter 127 127 22 0
gauss SetStandardDeviation 50.0
gauss SetMaximum 10000.0

vtkImageGradient gradient
gradient SetInput [gauss GetOutput]
gradient SetFilteredAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS
gradient ReleaseDataFlagOff

vtkImageEuclideanToPolar polar
polar SetInput [gradient GetOutput]
polar SetThetaMaximum 255

vtkImageViewer viewer
#viewer DebugOn
viewer SetInput [polar GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 255
viewer SetColorLevel 128
viewer SetRedComponent 0
viewer SetBlueComponent 1
viewer SetGreenComponent 1
viewer ColorFlagOn


#make interface
source WindowLevelInterface.tcl







