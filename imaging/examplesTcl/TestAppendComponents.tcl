catch {load vtktcl}
# A script to test the SphereSource

source vtkImageInclude.tcl

# Image pipeline

vtkImageSphereSource sphere1
sphere1 SetCenter 95 100 
sphere1 SetRadius 70

vtkImageSphereSource sphere2
sphere2 SetCenter 161 100 
sphere2 SetRadius 70

vtkImageSphereSource sphere3
sphere3 SetCenter 128 160 
sphere3 SetRadius 70

vtkImageAppendComponents append1
append1 SetInput1 [sphere1 GetOutput]
append1 SetInput2 [sphere2 GetOutput]

vtkImageAppendComponents append2
append2 SetInput1 [sphere3 GetOutput]
append2 SetInput2 [append1 GetOutput]
append2 ReleaseDataFlagOff

vtkImageViewer viewer
viewer SetInput [append2 GetOutput]
viewer SetColorWindow 255
viewer SetColorLevel 128
viewer ColorFlagOn

# make interface
source WindowLevelInterface.tcl







