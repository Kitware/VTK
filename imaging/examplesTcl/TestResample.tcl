catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# Doubles the The number of images (x dimension).


source vtkImageInclude.tcl


# Image pipeline

vtkImageReader reader
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "$VTK_DATA/fullHead/headsq"
reader SetDataMask 0x7fff
reader ReleaseDataFlagOff
#reader DebugOn

vtkImageResample magnify
magnify SetDimensionality 3
magnify SetInput [reader GetOutput]
magnify SetAxisOutputSpacing $VTK_IMAGE_X_AXIS 0.52
magnify SetAxisOutputSpacing $VTK_IMAGE_Y_AXIS 2.2
magnify SetAxisOutputSpacing $VTK_IMAGE_Z_AXIS 0.8
magnify ReleaseDataFlagOff
#magnify SetProgressMethod {set pro [magnify GetProgress]; puts "Completed $pro"; flush stdout}


vtkImageViewer viewer
viewer SetInput [magnify GetOutput]
viewer SetZSlice 30
viewer SetColorWindow 2000
viewer SetColorLevel 1000

# make interface
source WindowLevelInterface.tcl




