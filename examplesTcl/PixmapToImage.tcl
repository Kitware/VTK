catch {load vtktcl}

# get the interactor ui
source vtkInt.tcl

vtkPNMReader pnmReader
  pnmReader SetFileName "../../data/masonry.ppm"
  #pnmReader SetFileName "../../data/billBoard.pgm"

vtkImageXViewer viewer
  viewer SetAxes 0 1 4
  viewer SetColorWindow 160
  viewer SetColorLevel 80
  viewer ColorFlagOn
  viewer SetInput [pnmReader GetOutput]
  viewer Render

#make interface
#

frame .wl
frame .wl.f1
label .wl.f1.windowLabel -text Window
scale .wl.f1.window -from 1 -to 300 -orient horizontal -command SetWindow
frame .wl.f2
label .wl.f2.levelLabel -text Level
scale .wl.f2.level -from 1 -to 150 -orient horizontal -command SetLevel


.wl.f1.window set 160
.wl.f2.level set 80


pack .wl -side left
pack .wl.f1 .wl.f2 -side top
pack .wl.f1.windowLabel .wl.f1.window -side left
pack .wl.f2.levelLabel .wl.f2.level -side left


proc SetWindow window {
   global viewer
   viewer SetColorWindow $window
   viewer Render
}

proc SetLevel level {
   global viewer
   viewer SetColorLevel $level
   viewer Render
}


puts "Done"


