# append multiple displaced spheres into an RGB image.
catch {load vtktcl}
source vtkImageInclude.tcl

# Image pipeline

vtkImageElipsoidSource sphere1
sphere1 SetCenter 95 100 
sphere1 SetRadius 70 70 70

vtkImageElipsoidSource sphere2
sphere2 SetCenter 161 100 
sphere2 SetRadius 70 70 70 

vtkImageLogic xor
xor SetInput1 [sphere1 GetOutput]
xor SetInput2 [sphere2 GetOutput]
xor SetOutputTrueValue 150
xor SetOperationToXor

vtkImageViewer viewer
viewer SetInput [xor GetOutput]
viewer SetColorWindow 255
viewer SetColorLevel 128

# make interface
source WindowLevelInterface.tcl







