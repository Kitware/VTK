#
# Create a Pick Method that will export the current scsene to Renderman RIB format
#

vtkRIBExporter aRIB
set frame 1
proc Pick {} {
  global frame
  aRIB SetFilePrefix prman$frame
  aRIB SetTexturePrefix prman$frame
  aRIB BackgroundOn
  aRIB SetInput renWin
  aRIB Write
  set frame [expr $frame + 1]
}
iren SetEndPickMethod  {Pick}
