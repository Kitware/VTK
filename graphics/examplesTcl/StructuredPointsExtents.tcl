catch {load vtktcl}
#
# Regression test coutesy of Paul Hsieh, pashieh@usgs.gov
# get the interactor ui
source ../../examplesTcl/vtkInt.tcl

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

# Create the structured grid
vtkStructuredPoints spoints
  spoints SetDimensions 21  21  21 
  spoints SetOrigin -10 -10 -10
  spoints SetSpacing .1 .1 .1
  [spoints GetCellData] SetScalars scalars 

# Create the mapper and actor for the structrued grid
vtkDataSetMapper spointsMapper
  spointsMapper SetInput spoints 
  spointsMapper SetScalarRange 0.6  1.6 
vtkActor spointsActor
  spointsActor SetMapper spointsMapper 

ren1 AddActor spointsActor 

# Extract 3 sides of the structured grid
vtkStructuredPointsGeometryFilter geom1
  geom1 SetInput spoints 
  geom1 SetExtent 20  20  0  20  0  20 
vtkPolyDataMapper geom1Mapper
  geom1Mapper SetInput [geom1 GetOutput   ]
  geom1Mapper SetScalarRange 0.6  1.6 
vtkActor geom1Actor
  geom1Actor SetMapper geom1Mapper 
  geom1Actor AddPosition 2.5  0  0
  ren1 AddActor geom1Actor 


vtkStructuredPointsGeometryFilter geom2
  geom2 SetInput spoints 
  geom2 SetExtent 0  20  0  20  20  20 
vtkPolyDataMapper geom2Mapper
  geom2Mapper SetInput [geom2 GetOutput]
  geom2Mapper SetScalarRange 0.6  1.6 
vtkActor geom2Actor
  geom2Actor SetMapper geom2Mapper 
  geom2Actor AddPosition 2.5  0  0
  ren1 AddActor geom2Actor 

vtkStructuredPointsGeometryFilter geom3
  geom3 SetInput spoints 
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

renWin Render  
iren Initialize

iren SetUserMethod {wm deiconify .vtkInteract}

#renWin SetFileName "StructuredPointsExtents.tcl.ppm"
#renWin SaveImageAsPPM

proc TkCheckAbort {} {
  set foo [renWin GetEventPending]
  if {$foo != 0} {renWin SetAbortRender 1}
}
renWin SetAbortCheckMethod {TkCheckAbort}

# prevent the tk window from showing up then start the event loop
wm withdraw .
