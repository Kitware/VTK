catch {load vtktcl}
# Test the new reader


source vtkImageInclude.tcl


# Image pipeline

vtkImageFileReader reader
reader ReleaseDataFlagOff
reader SetDataMemoryOrder $VTK_IMAGE_COMPONENT_AXIS \
  $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS
reader SetDataDimensions 3 512 256
reader SetFileName "../../../data/earth.ppm"
reader SetDataScalarType $VTK_UNSIGNED_CHAR
#reader DebugOn


vtkImageViewer viewer
viewer SetInput [reader GetOutput]
viewer SetColorWindow 160
viewer SetColorLevel 80
viewer ColorFlagOn
viewer SetOriginLocationToUpperLeft

# make interface
source WindowLevelInterface.tcl



