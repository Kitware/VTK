catch {load vtktcl}

source vtkImageInclude.tcl

# This script uses a vtkTkRenderWidget to create a
# Tk widget that is associated with a vtkRenderWindow.
#source TkInteractor.tcl


# Create the GUI: two renderer widgets and a quit button
#
wm withdraw .
toplevel .top 

frame .top.f1 

vtkTkImageViewerWidget .top.f1.r1 -width 300 -height 300 
#    BindTkRenderWidget .top.f1.r1

button .top.btn  -text Quit -command exit

pack .top.f1.r1 -side left -padx 3 -pady 3 -fill both -expand t
pack .top.f1  -fill both -expand t
pack .top.btn -fill x

vtkImageFileReader reader
reader ReleaseDataFlagOff
reader SetAxes $VTK_IMAGE_COMPONENT_AXIS $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS 
reader SetDataDimensions 3 512 256
reader SetFileName "../../../data/earth.ppm"
reader SetDataScalarType $VTK_UNSIGNED_CHAR

set viewer [.top.f1.r1 GetImageViewer]
$viewer SetAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS $VTK_IMAGE_COMPONENT_AXIS
$viewer SetInput [reader GetOutput]
$viewer SetColorWindow 256
$viewer SetColorLevel 128
$viewer ColorFlagOn


