# append multiple displaced spheres into an RGB image.
catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

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

vtkImageAppendComponents appendImage
appendImage AddInput [sphere3 GetOutput]
appendImage AddInput [sphere1 GetOutput]
appendImage AddInput [sphere2 GetOutput]

vtkImageViewer viewer
viewer SetInput [appendImage GetOutput]
viewer SetColorWindow 255
viewer SetColorLevel 127.5

# make interface
source WindowLevelInterface.tcl







