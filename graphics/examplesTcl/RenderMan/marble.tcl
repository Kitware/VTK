#
# marble.tcl : requires LGVeinedMarble.sl
#
proc marble { veinfreq basecolor veincolor } {
  set name marble\_BaseColor_[lindex $basecolor 0]_[lindex $basecolor 1]_[lindex $basecolor 2]_VeinColor_[lindex $veincolor 0]_[lindex $veincolor 1]_[lindex $veincolor 2]
  if {[info commands $name] == ""} {
    vtkRIBProperty $name
      $name SetSurfaceShader LGVeinedMarble
      $name SetVariable veinfreq float
      $name AddVariable warpfreq float
      $name AddVariable veincolor color
      $name AddParameter veinfreq $veinfreq
      $name AddParameter veincolor $veincolor
      $name SetSpecularPower 5
      $name SetSpecular .5
      $name SetDiffuse 1
      eval $name SetDiffuseColor $basecolor
  }
  return $name
}
