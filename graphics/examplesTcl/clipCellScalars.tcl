catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

#
source $VTK_TCL/vtkInt.tcl

# Create the RenderWindow  Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Create a structured points data set
vtkStructuredPoints sp
  sp SetDimensions 11  11  1 
  sp SetSpacing 1  1  1 
  sp SetOrigin 0  0  0 

# Create some cell scalar data.
vtkScalars scalars
for {set i 0 } {$i < 100 } { incr i } {
  scalars InsertNextScalar [expr 0.01 * $i] 
  }

[sp GetCellData] SetScalars scalars 
scalars Delete  

# Use the geometry filter to make a poly data set.
vtkGeometryFilter geom
  geom SetInput sp 

# Define the clipping plane x = 2.5
vtkPlane plane
  plane SetNormal 1  0  0 
  plane SetOrigin 2.5  0  0 

# Clip the data with GenerateClippedOutputOn
vtkClipPolyData clip1
  clip1 SetInput [geom GetOutput]
  clip1 SetClipFunction plane 
  clip1 GenerateClippedOutputOn  

vtkPolyDataMapper mapper1
  mapper1 SetInput [clip1 GetOutput]
  mapper1 SetScalarModeToUseCellData  

vtkActor actor1
  actor1 SetMapper mapper1 

# Clip the same date with GenerateClippedOutputOff
vtkClipPolyData clip2
  clip2 SetInput [geom GetOutput]
  clip2 SetClipFunction plane 
  clip2 GenerateClippedOutputOff  

vtkPolyDataMapper mapper2
  mapper2 SetInput [clip2 GetOutput]
  mapper2 SetScalarModeToUseCellData  

vtkActor actor2
  actor2 SetMapper mapper2 
  actor2 AddPosition 12  0  0 

# Display the 2 actors
ren1 AddActor actor1 
ren1 AddActor actor2 

iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

renWin Render  
# prevent the tk window from showing up then start the event loop
wm withdraw .
