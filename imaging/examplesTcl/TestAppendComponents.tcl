# append multiple displaced spheres into an RGB image.
catch {load vtktcl}
source vtkImageInclude.tcl

# Image pipeline

vtkImageEllipsoidSource sphere1
sphere1 SetCenter 95 100 0
sphere1 SetRadius 70 70 70

vtkImageEllipsoidSource sphere2
sphere2 SetCenter 161 100 0
sphere2 SetRadius 70 70 70 

vtkImageEllipsoidSource sphere3
sphere3 SetCenter 128 160 0
sphere3 SetRadius 70 70 70

vtkImageAppendComponents append
append AddInput [sphere1 GetOutput]
append AddInput [sphere2 GetOutput]
append AddInput [sphere3 GetOutput]

vtkImageViewer viewer
viewer SetInput [append GetOutput]
viewer SetColorWindow 255
viewer SetColorLevel 127.5

# make interface
source WindowLevelInterface.tcl







