catch {load vtktcl}

set VTK_DATA "D:/vtkdata"

source D:/vtk/graphics/examplesTcl/TkInteractor.tcl
source D:/vtk/examplesTcl/vtkInt.tcl
source D:/vtk/examplesTcl/WidgetObject.tcl




# Create a line for display of the alpha
vtkPoints points
vtkCellArray alphaLine




# Starting point.
points InsertNextPoint 0.0 0.0 4.0
points InsertNextPoint 0.0 0.4 0.0
points InsertNextPoint 0.0 10.0 0.0
points InsertNextPoint 0.0 10.2 4.0

points InsertNextPoint 4.0 0.0 4.0
points InsertNextPoint 4.0 0.4 0.0
points InsertNextPoint 4.0 10.0 0.0
points InsertNextPoint 4.0 10.2 4.0

alphaLine InsertNextCell 4
alphaLine InsertCellPoint  0
alphaLine InsertCellPoint  1
alphaLine InsertCellPoint  2
alphaLine InsertCellPoint  3

alphaLine InsertNextCell 4
alphaLine InsertCellPoint  4
alphaLine InsertCellPoint  5
alphaLine InsertCellPoint  6
alphaLine InsertCellPoint  7


vtkPolyData data
    data SetPoints points
    data SetLines alphaLine

vtkTubeFilter tube
  tube SetNumberOfSides 10
  tube SetInput data
  tube SetRadius 1.0
  tube CappingOn




vtkPolyDataMapper mapper1
  mapper1 SetInput [tube GetOutput]
vtkActor actor1 
  actor1 SetMapper mapper1
  vtkProperty bfp
  actor1 SetBackfaceProperty bfp
  [actor1 GetProperty] SetColor 1.0 0.6 0.6
  [actor1 GetBackfaceProperty] SetColor 0.0 1.0 1.0
  [actor1 GetBackfaceProperty] SetAmbient 0.5
  [actor1 GetBackfaceProperty] SetDiffuse 0.5
  [actor1 GetProperty] SetAmbient 0.5
  [actor1 GetProperty] SetDiffuse 0.5

vtkRenderer ren1
vtkRenderWindow renWin
    renWin SetDesiredUpdateRate 20
    renWin AddRenderer ren1


ren1 AddActor actor1
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 400 300

# render the image
#
set cam1 [ren1 GetActiveCamera]
$cam1 Zoom 1.4




# Create the GUI: two renderer widgets and a quit button
#
wm withdraw .
toplevel .top 

frame .top.f1 

vtkTkRenderWidget .top.f1.r1 -width 500 -height 500 -rw renWin
button .top.btn  -text Quit -command exit

pack .top.f1.r1 -side left -padx 3 -pady 3 -fill both -expand t
#pack .top.f1.r2 -side left -padx 3 -pady 3 -fill both -expand t
pack .top.f1  -fill both -expand t
pack .top.btn -fill x



BindTkRenderWidget .top.f1.r1

vtkWorldPointPicker picker
bind .top.f1.r1 <Shift-Button-1> {pick %x %y}
bind .top.f1.r1 <Shift-Button1-Motion> {move}
bind .top.f1.r1 <Shift-ButtonRelease-1> {stop}

bind .top.f1.r1 <n> {collide TestCollisionForce; renWin Render}

