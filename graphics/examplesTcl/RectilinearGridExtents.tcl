catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

#
# Regression test coutesy of Paul Hsieh, pashieh@usgs.gov
# get the interactor ui
source $VTK_TCL/vtkInt.tcl

# Create the rendering stuff
vtkRenderer ren1
vtkRenderWindow renWin
  renWin AddRenderer ren1 
vtkRenderWindowInteractor iren
  iren SetRenderWindow renWin 

# Create scalars for cells
vtkScalars scalars
  scalars SetNumberOfScalars [expr 20 * 20 * 20] 
set n  0
for {set k 0} {$k < 20} {incr k} {
  set z [expr 0.1 * ($k - 10) ]
  for {set j 0} {$j < 20} {incr j} {
      set y [expr 0.1 * ($j - 10) + .05]
      for {set i 0} {$i < 20} {incr i} {
        set x [expr 0.1 * ($i - 10) + .05]
	set s [expr sqrt($x*$x + $y*$y + $z*$z)]
        scalars SetScalar $n  $s
        incr n
      }
    }
}

set n  0
vtkFloatArray sxArray
for {set i 0} {$i < 22} {incr i} {
 set x [expr 0.1 * ($i - 10)]
 sxArray InsertNextValue $x
}

# Create the rectilinear grid
vtkRectilinearGrid rgrid
  rgrid SetDimensions 21  21  21 
  rgrid SetXCoordinates sxArray
  rgrid SetYCoordinates sxArray
  rgrid SetZCoordinates sxArray
  [rgrid GetCellData] SetScalars scalars 

# Create the mapper and actor for the structrued grid
vtkDataSetMapper rgridMapper
  rgridMapper SetInput rgrid 
  rgridMapper SetScalarRange 0.6  1.6 
vtkActor rgridActor
  rgridActor SetMapper rgridMapper 

ren1 AddActor rgridActor 

# Extract 3 sides of the structured grid
vtkRectilinearGridGeometryFilter geom1
  geom1 SetInput rgrid 
  geom1 SetExtent 20  20  0  20  0  20 
vtkPolyDataMapper geom1Mapper
  geom1Mapper SetInput [geom1 GetOutput   ]
  geom1Mapper SetScalarRange 0.6  1.6 
vtkActor geom1Actor
  geom1Actor SetMapper geom1Mapper 
  geom1Actor AddPosition 2.5  0  0
  ren1 AddActor geom1Actor 


vtkRectilinearGridGeometryFilter geom2
  geom2 SetInput rgrid 
  geom2 SetExtent 0  20  0  20  20  20 
vtkPolyDataMapper geom2Mapper
  geom2Mapper SetInput [geom2 GetOutput]
  geom2Mapper SetScalarRange 0.6  1.6 
vtkActor geom2Actor
  geom2Actor SetMapper geom2Mapper 
  geom2Actor AddPosition 2.5  0  0
  ren1 AddActor geom2Actor 

vtkRectilinearGridGeometryFilter geom3
  geom3 SetInput rgrid 
  geom3 SetExtent 0  20  20  20  0  20 
vtkPolyDataMapper geom3Mapper
  geom3Mapper SetInput [geom3 GetOutput]
  geom3Mapper SetScalarRange 0.6  1.6 
vtkActor geom3Actor
  geom3Actor SetMapper geom3Mapper 
  geom3Actor AddPosition 2.5  0  0
  ren1 AddActor geom3Actor 

renWin SetSize 300 300 
[ren1 GetActiveCamera] Azimuth 30
[ren1 GetActiveCamera] Elevation 40
[ren1 GetActiveCamera] Dolly 1.25
ren1 ResetCameraClippingRange

renWin Render  
iren Initialize

iren SetUserMethod {wm deiconify .vtkInteract}

#renWin SetFileName "RectilinearGridExtents.tcl.ppm"
#renWin SaveImageAsPPM

proc TkCheckAbort {} {
  set foo [renWin GetEventPending]
  if {$foo != 0} {renWin SetAbortRender 1}
}
renWin SetAbortCheckMethod {TkCheckAbort}

# prevent the tk window from showing up then start the event loop
wm withdraw .
