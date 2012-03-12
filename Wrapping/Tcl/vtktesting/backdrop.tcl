proc BuildBackdrop {minX maxX minY maxY minZ maxZ thickness} {
    if { [info command basePlane] == "" } {vtkCubeSource basePlane;}
    basePlane SetCenter [expr ($maxX + $minX) / 2.0] $minY [expr ($maxZ + $minZ) / 2.0]
    basePlane SetXLength [expr ($maxX - $minX)]
    basePlane SetYLength $thickness
    basePlane SetZLength [expr ($maxZ - $minZ)]

    if { [info command baseMapper] == "" } {vtkPolyDataMapper baseMapper;}
     baseMapper SetInputConnection [basePlane GetOutputPort]
    if { [info command base] == "" } {vtkActor base;}
     base SetMapper baseMapper

    if { [info command backPlane] == "" } {vtkCubeSource backPlane;}
    backPlane SetCenter [expr ($maxX + $minX) / 2.0] [expr ($maxY + $minY) / 2.0]  $minZ
    backPlane SetXLength [expr ($maxX - $minX)]
    backPlane SetYLength [expr ($maxY - $minY)]
    backPlane SetZLength $thickness

    if { [info command backMapper] == "" } {vtkPolyDataMapper backMapper;}
     backMapper SetInputConnection [backPlane GetOutputPort]
    if { [info command back] == "" } {vtkActor back;}
   back SetMapper backMapper

    if { [info command leftPlane] == "" } {vtkCubeSource leftPlane;}
    leftPlane SetCenter $minX [expr ($maxY + $minY) / 2.0] [expr ($maxZ + $minZ) / 2.0]
    leftPlane SetXLength $thickness
    leftPlane SetYLength [expr ($maxY - $minY)]
    leftPlane SetZLength [expr ($maxZ - $minZ)]

    if { [info command leftMapper] == "" } {vtkPolyDataMapper leftMapper;}
     leftMapper SetInputConnection [leftPlane GetOutputPort]
    if { [info command left] == "" } {vtkActor left;}
     left SetMapper leftMapper

}
