catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# Source the interactor that we will use for the TkRenderWidget
source $VTK_TCL/../graphics/examplesTcl/TkInteractor.tcl

# user interface command widget
source $VTK_TCL/vtkInt.tcl



# ------------------- Create the UI ---------------------

# UI Variables
set Piece 2
set NumberOfPieces 13
set GhostLevels 6



# prevent the tk window from showing up then start the event loop
wm withdraw .

# Create the toplevel window
toplevel .top
wm title .top {Unstructured/Polydata Piece/GhostCell Demostration}

# Create some frames
frame .top.f1 
frame .top.f2
pack .top.f1 .top.f2 -side top -expand 1 -fill both

vtkRenderWindow renWin
vtkTkRenderWidget .top.f1.rw -width 400 -height 400 -rw renWin
BindTkRenderWidget .top.f1.rw
pack .top.f1.rw -expand 1 -fill both

# create a rendering window and renderer
vtkRenderer ren1
    renWin AddRenderer ren1


scale .top.f2.s1 -label " Number Of Pieces: " -orient horizontal \
	-length 200 -from 1 -to 52 -variable NumberOfPieces 
scale .top.f2.s2 -label " Piece: " -orient horizontal \
	-length 200 -from 0 -to [expr $NumberOfPieces - 1] -variable Piece 
scale .top.f2.s3 -label " Ghosts Levels: " -orient horizontal \
	-length 200 -from 0 -to 19 -variable GhostLevels 

pack .top.f2.s1 .top.f2.s2 .top.f2.s3 -side top -expand 1 -fill both

button .top.f2.b1 -text "Quit" -command {exit}
pack .top.f2.b1  -expand 1 -fill x

bind .top.f2.s1 <ButtonRelease> { 
   mapper SetNumberOfPieces $NumberOfPieces
   pointMapper SetNumberOfPieces $NumberOfPieces
   if {$Piece >= $NumberOfPieces} {
      set Piece [expr $NumberOfPieces - 1]
      mapper SetPiece $Piece
      pointMapper SetPiece $Piece
   }
   .top.f2.s2 configure -to [expr $NumberOfPieces - 1]
   renWin Render
}

bind .top.f2.s2 <ButtonRelease> { 
   mapper SetPiece $Piece
   pointMapper SetPiece $Piece
   renWin Render
}

bind .top.f2.s3 <ButtonRelease> { 
   mapper SetGhostLevel $GhostLevels
   pointMapper SetGhostLevel $GhostLevels
   mapper SetScalarRange 0 [expr $GhostLevels + 0.1]
   pointMapper SetScalarRange 0 [expr $GhostLevels + 0.1]
   renWin Render
}









vtkSphereSource sphere
  sphere SetThetaResolution 24
  sphere SetPhiResolution 15







vtkExtractPolyDataPiece pieceFilter
    pieceFilter SetInput [sphere GetOutput]

# Move ghost levels to scalars.
# First move all attributes to field data.
vtkAttributeDataToFieldDataFilter cd2fd
    cd2fd SetInput [pieceFilter GetOutput]
# Move ghost cell field to cell scalars 
vtkFieldDataToAttributeDataFilter fd2cd
    fd2cd SetInput [cd2fd GetOutput]
    fd2cd SetInputFieldToCellDataField
    fd2cd SetOutputAttributeDataToCellData
    fd2cd SetScalarComponent 0 CellGhostLevels 0 
# Move all attributes to field data again because we lost the field data.
vtkFieldDataToAttributeDataFilter fd2pd
    fd2pd SetInput [fd2cd GetOutput]
    fd2pd SetInputFieldToPointDataField
    fd2pd SetOutputAttributeDataToPointData
    fd2pd SetScalarComponent 0 PointGhostLevels 0 




vtkPolyDataMapper mapper
  mapper SetInput [fd2pd GetOutput]
  mapper SetPiece $Piece
  mapper SetNumberOfPieces $NumberOfPieces
  mapper SetGhostLevel $GhostLevels
  mapper SetScalarRange 0 $GhostLevels
  mapper SetColorModeToMapScalars 
  mapper SetScalarModeToUsePointData
  mapper SetScalarModeToUseCellData
vtkActor actor
  actor SetMapper mapper





vtkSphereSource testSphere
testSphere Update
set pd [testSphere GetOutput]


mapper Update


vtkPolyData pdCopy
  pdCopy ShallowCopy [mapper GetInput]
  #pdCopy ShallowCopy $pd


# lets show the points using glyph
vtkSphereSource glyphSource
    glyphSource SetThetaResolution 8
    glyphSource SetPhiResolution 5
vtkGlyph3D glyph
    #glyph SetInput pdCopy
    glyph SetInput [fd2pd GetOutput]
    glyph SetSource [glyphSource GetOutput]
    glyph SetScaleModeToDataScalingOff
    glyph SetScaleFactor 0.03

vtkPolyDataMapper pointMapper
    pointMapper SetInput [glyph GetOutput]
    pointMapper SetPiece $Piece
    pointMapper SetNumberOfPieces $NumberOfPieces
    pointMapper SetGhostLevel $GhostLevels
    pointMapper SetScalarRange 0 $GhostLevels
    pointMapper SetColorModeToMapScalars 
vtkActor pointActor
    pointActor SetMapper pointMapper




# Add the actors to the renderer, set the background and size
#
ren1 AddActor actor
ren1 AddActor pointActor

ren1 SetBackground 1 1 1









