#
# Create a Pick Method that will export the current scsene to Renderman RIB format
#

vtkRIBExporter aRIB
set RIBPrefix "prman"
set frame 1
proc Pick {} {
  global frame RIBPrefix
  aRIB SetFilePrefix $RIBPrefix$frame
  aRIB SetTexturePrefix $RIBPrefix$frame
  aRIB BackgroundOn
  aRIB SetInput renWin
  aRIB Write
  set frame [expr $frame + 1]
}
iren SetEndPickMethod  {Pick}
