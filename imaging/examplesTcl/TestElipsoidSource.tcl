catch {load vtktcl}
# A script to test the ElipsoidSource

source vtkImageInclude.tcl

# Image pipeline

vtkImageElipsoidSource elipsoid
elipsoid SetWholeExtent 0 225 0 225 0 20
elipsoid SetCenter 100 100 10
elipsoid SetRadius 100 80 20
elipsoid SetOutValue 150
elipsoid SetInValue 70
elipsoid SetOutputScalarType $VTK_SHORT
elipsoid ReleaseDataFlagOff

puts [elipsoid Print]

vtkImageViewer viewer
viewer SetInput [elipsoid GetOutput]
viewer SetZSlice 10
viewer SetColorWindow 255
viewer SetColorLevel 128
#viewer DebugOn

# make interface
source WindowLevelInterface.tcl







