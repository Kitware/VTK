catch {load vtktcl}
# A script to test the SinusoidSource

source vtkImageInclude.tcl

# Image pipeline

vtkImageSinusoidSource cos
cos SetWholeExtent 0 225 0 225 0 20 0 0
cos SetAmplitude 250
cos SetDirection 1 1 1
cos SetPeriod 20
cos ReleaseDataFlagOff

vtkImageViewer viewer
viewer SetInput [cos GetOutput]
viewer SetZSlice 10
viewer SetColorWindow 255
viewer SetColorLevel 128
#viewer DebugOn

# make interface
source WindowLevelInterface.tcl







