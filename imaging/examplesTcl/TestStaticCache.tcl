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
set range [staticCache GetScalarRange]
set min [lindex $range 0]
set max [lindex $range 1]


vtkImageViewer viewer
viewer SetInput staticCache
viewer SetColorWindow [expr $max - $min]
viewer SetColorLevel [expr ($max + $min) * 0.5]
viewer Render

viewer SetPosition 50 50

#make interface
source WindowLevelInterface.tcl







