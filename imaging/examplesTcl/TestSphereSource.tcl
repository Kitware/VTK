catch {load vtktcl}
# A script to test the SphereSource

source vtkImageInclude.tcl

# Image pipeline

vtkImageSphereSource sphere
sphere SetWholeExtent 0 210 0 210 0 256
sphere SetCenter 100 100 100
sphere SetRadius 110
sphere SetOutValue 250
sphere SetInValue 50
sphere SetOutputScalarType $VTK_SHORT
sphere ReleaseDataFlagOff

vtkImageViewer viewer
viewer SetInput [sphere GetOutput]
viewer SetZSlice 50
viewer SetColorWindow 255
viewer SetColorLevel 128
#viewer DebugOn

# make interface
source WindowLevelInterface.tcl







