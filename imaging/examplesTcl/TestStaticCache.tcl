catch {load vtktcl}
# I do not line the way this object works, but here is a regresion test.


source vtkImageInclude.tcl

# Image pipeline
vtkPNMReader reader
reader ReleaseDataFlagOff
reader SetFileName "../../../vtkdata/earth.ppm"


vtkImageStaticCache staticCache
set data [[reader GetOutput] UpdateAndReturnData]
staticCache SetCachedData $data


vtkImageViewer viewer
viewer SetInput staticCache
viewer SetColorWindow 255
viewer SetColorLevel 128
viewer Render

viewer SetPosition 50 50

#make interface
source WindowLevelInterface.tcl







