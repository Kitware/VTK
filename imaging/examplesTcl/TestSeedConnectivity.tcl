catch {load vtktcl}
# A script to test the island removal filter.
# first the image is thresholded, then small islands are removed.

source vtkImageInclude.tcl

# Image pipeline

vtkImageReader reader
reader ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff
#reader DebugOn
#reader Update

vtkImageThreshold thresh
thresh SetInput [reader GetOutput]
thresh SetOutputScalarType $VTK_UNSIGNED_CHAR
thresh ThresholdByUpper 2000.0
thresh SetInValue 255
thresh SetOutValue 0
thresh ReleaseDataFlagOff

vtkImageSeedConnectivity connect
connect SetInput [thresh GetOutput]
connect SetInputConnectValue 255
connect SetOutputConnectedValue 255
connect SetOutputUnconnectedValue 128
#connect SetFilteredAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_X_AXIS
#connect AddSeed 0 200
connect SetFilteredAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS $VTK_IMAGE_Z_AXIS 
connect AddSeed 0 200 22

vtkImageViewer viewer
viewer SetInput [connect GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 255
viewer SetColorLevel 128
#viewer DebugOn

# make interface
source WindowLevelInterface.tcl







