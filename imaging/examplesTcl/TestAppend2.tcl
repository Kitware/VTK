catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }


# This test the PreserveExtents feature of the ImageAppend filter.


source vtkImageInclude.tcl
source TkImageViewerInteractor.tcl


# This script tests the Append filter that puts two images together.

vtkPNMReader reader1
reader1 ReleaseDataFlagOff
reader1 SetFileName "$VTK_DATA/earth.ppm"

vtkPNMReader reader2
reader2 ReleaseDataFlagOff
reader2 SetFileName "$VTK_DATA/masonry.ppm"

vtkImageTranslateExtent trans
  trans SetTranslation 80 50 0
  trans SetInput [reader2 GetOutput]


vtkImageAppend appendF
appendF PreserveExtentsOn
appendF AddInput [reader1 GetOutput]
appendF AddInput [trans GetOutput]

vtkImageViewer viewer
viewer SetInput [appendF GetOutput]
viewer SetColorWindow 256
viewer SetColorLevel 127.5

# Create the GUI: two renderer widgets and a quit button
#
wm withdraw .
toplevel .top 

frame .top.f1 

vtkTkImageViewerWidget .top.f1.r1 -width 512 -height 306 -iv viewer
#    BindTkRenderWidget .top.f1.r1

button .top.btn  -text Quit -command exit

pack .top.f1.r1 -side left -padx 3 -pady 3 -fill both -expand t
pack .top.f1  -fill both -expand t
pack .top.btn -fill x


BindTkImageViewer .top.f1.r1 

