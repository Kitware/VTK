# Halves the size of the image in the x, Y and Z dimensions.

catch {load vtktcl}
source vtkImageInclude.tcl


# Image pipeline

vtkImageReader reader
#reader DebugOn
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff
#[reader GetOutput] DebugOn

vtkImageShrink3D shrink
shrink DebugOn
shrink SetInput [reader GetOutput]
shrink SetShrinkFactors 2 2 2
puts [shrink GetShrinkFactors]
#shrink SetProgressMethod {set pro [shrink GetProgress]; puts "Completed $pro"; flush stdout}
#shrink Update
shrink SetNumberOfThreads 1
#[shrink GetOutput] DebugOn

vtkImageViewer viewer
#viewer DebugOn
<<<<<<< TestShrink3D.tcl
viewer SetAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS $VTK_IMAGE_Z_AXIS
puts "[[shrink GetOutput] Print]"
=======
>>>>>>> 1.17
viewer SetInput [shrink GetOutput]
viewer SetZSlice 11
viewer SetColorWindow 2000
viewer SetColorLevel 1000


# make interface
source WindowLevelInterface.tcl







