catch {load vtktcl}
# user interface command widget
source ../../examplesTcl/vtkInt.tcl

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
