catch {load vtktcl}
# A script to test the ElipsoidSource

source vtkImageInclude.tcl

# Image pipeline

vtkImageGaussianSource gauss
gauss SetWholeExtent 0 225 0 225 0 20 0 0
gauss SetCenter 100 100 10 0
gauss SetStandardDeviation 100.0
gauss SetMaximum 255.0
gauss ReleaseDataFlagOff

vtkImageViewer viewer
viewer SetInput [gauss GetOutput]
viewer SetZSlice 10
viewer SetColorWindow 255
viewer SetColorLevel 128
#viewer DebugOn

# make interface
source WindowLevelInterface.tcl







