catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# user interface command widget
source $VTK_TCL/vtkInt.tcl

# create a rendering window and renderer
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
    renWin StereoCapableWindowOn  

# create an actor and give it cone geometry
vtkConeSource cone
  cone SetResolution 8
vtkPolyDataMapper coneMapper
  coneMapper SetInput [cone GetOutput]
vtkActor coneActor
  coneActor SetMapper coneMapper

# assign our actor to the renderer
ren1 AddActor coneActor
renWin Render

vtkWindowToImageFilter w2if
  w2if SetInput renWin

vtkImageDifference imgDiff
    
vtkPNMReader rtpnm
  rtpnm SetFileName "valid/Cone.tcl.ppm"

imgDiff SetInput [w2if GetOutput]
imgDiff SetImage [rtpnm GetOutput]
imgDiff Update

if {[imgDiff GetThresholdedError] <= 10 } {
        puts "Tcl smoke test passed."
} else {
        puts "Tcl smoke test failed."
}	

wm withdraw .

exit
