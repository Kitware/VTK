# Demonstrate us of glyph table
catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# get the interactor
source $VTK_TCL/vtkInt.tcl

# read the data
set f [open "$VTK_DATA/bpa.mol" r]
set i 0
while { [gets $f line] >=0 } {
    scan $line "%f %f %f %f %d" at($i,x) at($i,y) at($i,z) at($i,r) at($i,t)
    incr i
}
close $f

set x "x"
set y "y"
set z "z"

set natom $i

# Create some glyphs 
vtkSphereSource atom0
    atom0 SetPhiResolution 18
    atom0 SetThetaResolution 18
    atom0 SetRadius 1
vtkStripper fastAtom0
  fastAtom0 SetInput [atom0 GetOutput]

vtkSphereSource atom1
    atom1 SetPhiResolution 18
    atom1 SetThetaResolution 18
    atom1 SetRadius 1
vtkStripper fastAtom1
  fastAtom1 SetInput [atom1 GetOutput]

vtkSphereSource atom2
    atom2 SetPhiResolution 18
    atom2 SetThetaResolution 18
    atom2 SetRadius 1.50
vtkStripper fastAtom2
  fastAtom2 SetInput [atom2 GetOutput]

vtkSphereSource atom3
    atom3 SetPhiResolution 18
    atom3 SetThetaResolution 18
    atom3 SetRadius 1.85
vtkStripper fastAtom3
  fastAtom3 SetInput [atom3 GetOutput]

vtkSphereSource atom4
    atom4 SetPhiResolution 18
    atom4 SetThetaResolution 18
    atom4 SetRadius 1
vtkStripper fastAtom4
  fastAtom4 SetInput [atom4 GetOutput]

vtkSphereSource atom5
    atom5 SetPhiResolution 18
    atom5 SetThetaResolution 18
    atom5 SetRadius 1.65
vtkStripper fastAtom5
  fastAtom5 SetInput [atom5 GetOutput]

# These are the points and scalars to glyph
vtkPoints points
vtkScalars scalars

for {set i 0} {$i < $natom} {incr i} {
    points InsertPoint $i $at($i,x) $at($i,y) $at($i,z)
    scalars InsertScalar $i $at($i,t)
}
vtkPolyData molecule
    molecule SetPoints points
    [molecule GetPointData] SetScalars scalars

# Create the table of glyphs
vtkGlyph3D glyphs
    glyphs SetInput molecule
    glyphs SetNumberOfSources 6
    glyphs SetSource 0 [fastAtom0 GetOutput]
    glyphs SetSource 1 [fastAtom1 GetOutput]
    glyphs SetSource 2 [fastAtom2 GetOutput]
    glyphs SetSource 3 [fastAtom3 GetOutput]
    glyphs SetSource 4 [fastAtom4 GetOutput]
    glyphs SetSource 5 [fastAtom5 GetOutput]
    glyphs SetIndexModeToScalar
    glyphs SetRange 0 5
    glyphs ScalingOff


# Color the glyphs - the scalar values are indexed into the table
vtkLookupTable lut
    lut SetNumberOfColors 6
    lut Build
    lut SetTableValue 0 0 0 0 0
    lut SetTableValue 1 1 0 0 1
    lut SetTableValue 2 0 1 0 1
    lut SetTableValue 3 0 0 1 1
    lut SetTableValue 4 1 1 0 1
    lut SetTableValue 5 1 0 1 1
vtkPolyDataMapper atommapper
    atommapper SetInput [glyphs GetOutput]
    atommapper SetScalarRange 0 5
    atommapper SetLookupTable lut
vtkProperty atomprop
    atomprop SetColor 1 1 0
    atomprop SetDiffuse .7
    atomprop SetSpecular .5
    atomprop SetSpecularPower 30
vtkActor atomactor
    atomactor SetMapper atommapper
    atomactor SetProperty atomprop

# Create graphics stuff
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 AddActor atomactor     

# Setup view
[ren1 GetActiveCamera] Azimuth -60
[ren1 GetActiveCamera] Dolly 1.5
ren1 ResetCameraClippingRange

ren1 SetBackground .8 .8 .8
renWin SetSize 400 400

iren Initialize
iren SetUserMethod {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .
