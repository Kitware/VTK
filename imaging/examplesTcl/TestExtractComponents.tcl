catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# Get Vectors from the gradent, and extract the z component.



source vtkImageInclude.tcl


# Image pipeline

#vtkImageReader reader
#reader DebugOn
#reader SetDataByteOrderToLittleEndian
#reader SetDataExtent 0 255 0 255 1 93
#reader SetFilePrefix "$VTK_DATA/fullHead/headsq"
#reader SetDataMask 0x7fff

#vtkImageGradient gradient
#gradient SetInput [reader GetOutput]
#gradient SetFilteredAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS $VTK_IMAGE_Z_AXIS

vtkPNMReader reader
reader SetFileName "$VTK_DATA/masonry.ppm"

vtkImageExtractComponents extract
extract SetInput [reader GetOutput]
extract SetComponents 0 1 2
extract ReleaseDataFlagOff

vtkImageViewer viewer
#viewer DebugOn
viewer SetInput [extract GetOutput]
viewer SetZSlice 0
viewer SetColorWindow 800
viewer SetColorLevel 0


#make interface
source WindowLevelInterface.tcl







