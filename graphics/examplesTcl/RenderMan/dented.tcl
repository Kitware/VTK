#
# requires dented.sl
#
proc dented { Km } {
  if {[info commands dented$Km] == ""} {
    vtkRIBProperty dented$Km
      dented$Km SetVariable Km float
      dented$Km SetParameter Km $Km
      dented$Km SetDisplacementShader dented
      dented$Km SetSurfaceShader plastic
    }
  return dented$Km
}




