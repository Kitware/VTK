catch {load vtktcl}
# Simple viewer for images.

source ../../imaging/examplesTcl/vtkImageInclude.tcl

# Image pipeline

vtkImageReader reader
reader ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetDataOrigin -127.5 -127.5 -47
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff
reader UpdateWholeExtent

vtkTransform transform
transform RotateX 10
transform RotateY 20
transform RotateZ 30

#vtkMatrix4x4 matrix
#matrix SetElement 3 0 0.001
#matrix SetElement 3 1 0.001
#matrix SetElement 3 2 0.001
#transform Concatenate matrix

vtkImageReslice reslice
reslice SetInput [reader GetOutput]
reslice SetResliceTransform transform
reslice InterpolateOn
reslice SetBackgroundLevel 1023
#reslice SetOutputExtent 0 300 0 300 1 93
#reslice SetOutputOrigin -127.5 -127.5 -47
#reslice SetOutputSpacing 0.5 0.5 0.5
#reslice UpdateWholeExtent

vtkImageViewer viewer
viewer SetInput [reslice GetOutput]
viewer SetZSlice 50
viewer SetColorWindow 2000
viewer SetColorLevel 1000
#viewer DebugOn
#viewer Render

#source  /vtk/reflect-1.2.tcl
#reflect reslice reader transform

source ../../imaging/examplesTcl/WindowLevelInterface.tcl














