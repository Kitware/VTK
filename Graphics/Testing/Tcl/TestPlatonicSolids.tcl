package require vtk
package require vtkinteraction
package require vtktesting

# Create five instances of vtkPlatonicSolidSource 
# corresponding to each of the five Platonic solids.
#
vtkPlatonicSolidSource tet
  tet SetSolidTypeToTetrahedron
vtkPolyDataMapper tetMapper
  tetMapper SetInputConnection [tet GetOutputPort]
vtkActor tetActor
  tetActor SetMapper tetMapper

vtkPlatonicSolidSource cube
  cube SetSolidTypeToCube
vtkPolyDataMapper cubeMapper
  cubeMapper SetInputConnection [cube GetOutputPort]
vtkActor cubeActor
  cubeActor SetMapper cubeMapper
  cubeActor AddPosition 2.0 0 0

vtkPlatonicSolidSource oct
  oct SetSolidTypeToOctahedron
vtkPolyDataMapper octMapper
  octMapper SetInputConnection [oct GetOutputPort]
vtkActor octActor
  octActor SetMapper octMapper
  octActor AddPosition 4.0 0 0

vtkPlatonicSolidSource icosa
  icosa SetSolidTypeToIcosahedron
vtkPolyDataMapper icosaMapper
  icosaMapper SetInputConnection [icosa GetOutputPort]
vtkActor icosaActor
  icosaActor SetMapper icosaMapper
  icosaActor AddPosition 6.0 0 0

vtkPlatonicSolidSource dode
  dode SetSolidTypeToDodecahedron
vtkPolyDataMapper dodeMapper
  dodeMapper SetInputConnection [dode GetOutputPort]
vtkActor dodeActor
  dodeActor SetMapper dodeMapper
  dodeActor AddPosition 8.0 0 0

# Create rendering stuff
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor tetActor
ren1 AddActor cubeActor
ren1 AddActor octActor
ren1 AddActor icosaActor
ren1 AddActor dodeActor

# Create a lookup table with colors for each face
#
vtkMath math
vtkLookupTable lut
  lut SetNumberOfColors 20
  lut Build
  lut SetTableValue 0 1 0 0 1
  lut SetTableValue 1 0 1 0 1
  lut SetTableValue 2 1 1 0 1
  lut SetTableValue 3 0 0 1 1
  lut SetTableValue 4 1 0 1 1
  lut SetTableValue 5 0 1 1 1
  eval lut SetTableValue 6 $spring_green 1.0
  eval lut SetTableValue 7 $lavender 1.0
  eval lut SetTableValue 8 $mint_cream 1.0
  eval lut SetTableValue 9 $violet 1.0
  eval lut SetTableValue 10 $ivory_black 1.0
  eval lut SetTableValue 11 $coral 1.0
  eval lut SetTableValue 12 $pink 1.0
  eval lut SetTableValue 13 $salmon 1.0
  eval lut SetTableValue 14 $sepia 1.0
  eval lut SetTableValue 15 $carrot 1.0
  eval lut SetTableValue 16 $gold 1.0
  eval lut SetTableValue 17 $forest_green 1.0
  eval lut SetTableValue 18 $turquoise 1.0
  eval lut SetTableValue 19 $plum 1.0

lut SetTableRange 0 19
tetMapper SetLookupTable lut
tetMapper SetScalarRange 0 19
cubeMapper SetLookupTable lut
cubeMapper SetScalarRange 0 19
octMapper SetLookupTable lut
octMapper SetScalarRange 0 19
icosaMapper SetLookupTable lut
icosaMapper SetScalarRange 0 19
dodeMapper SetLookupTable lut
dodeMapper SetScalarRange 0 19

set cam [ren1 GetActiveCamera] 
$cam SetPosition 3.89696 7.20771 1.44123
$cam SetFocalPoint 3.96132 0 0
$cam SetViewUp -0.0079335 0.196002 -0.980571
$cam SetClippingRange 5.42814 9.78848

ren1 SetBackground 0 0 0
renWin SetSize 400 150
iren Initialize

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .
