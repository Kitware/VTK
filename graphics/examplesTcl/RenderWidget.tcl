catch {load vtktcl}
catch {load vtktcl}
# This script uses a vtkTkRenderWidget to create a
# Tk widget that is associated with a vtkRenderWindow.
# It only works for X windows right now.


wm withdraw .
toplevel .top -visual {truecolor 24}
frame .top.f1



vtkTkRenderWidget .top.f1.o1 -width 300 -height 300 
button .top.btn  -text Quit -command exit
bind .top.f1.o1 <Any-ButtonPress> {puts "button %b press (%x, %y)"}
bind .top.f1.o1 <Any-ButtonRelease> {puts "button %b release (%x, %y)"}
bind .top.f1.o1 <B1-Motion> {puts "B%b-Motion (%x, %y)"}
bind .top.f1.o1 <B2-Motion> {puts "B%b-Motion (%x, %y)"}
bind .top.f1.o1 <B3-Motion> {puts "B%b-Motion (%x, %y)"}

puts [bind .top.f1.o1]

pack .top.f1.o1 -side left -padx 3 -pady 3 -fill both -expand t
pack .top.f1  -fill both -expand t
pack .top.btn -fill x



# Get the render window associated with the widget.
set renWin [.top.f1.o1 GetRenderWindow]
set ren1   [$renWin MakeRenderer]

# create a sphere source and actor
#
vtkSphereSource sphere
vtkPolyMapper   sphereMapper
    sphereMapper SetInput [sphere GetOutput]
vtkLODActor sphereActor
    sphereActor SetMapper sphereMapper

# create the spikes using a cone source and the sphere source
#
vtkConeSource cone
vtkGlyph3D glyph
    glyph SetInput [sphere GetOutput]
    glyph SetSource [cone GetOutput]
    glyph UseNormal
    glyph ScaleByVector
    glyph SetScaleFactor 0.25
vtkPolyMapper spikeMapper
    spikeMapper SetInput [glyph GetOutput]
vtkLODActor spikeActor
    spikeActor SetMapper spikeMapper

# Add the actors to the renderer, set the background and size
#
$ren1 AddActors sphereActor
$ren1 AddActors spikeActor
$ren1 SetBackground 0.1 0.2 0.4
$renWin SetSize 300 300









