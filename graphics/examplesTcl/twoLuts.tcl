#
# This pgm illustrates a bug with using ImmediateModeRenderingOn together
# with an explicit LUT for two distinct mappers - using one or the other
# is OK  but used together there's a problem in that one actor's mapper
# overrides the other's.
#

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

# ----  explicitly define default LUT
vtkLookupTable lut
  lut Build  

  # ----- create line #1 ---------
  vtkPoints pts1
    pts1 InsertPoint 0  -1 0 0 
    pts1 InsertPoint 1   0 0 0 
    pts1 InsertPoint 2   1 0 0 

  vtkScalars scalars1
    scalars1 InsertScalar 0  0.0 
    scalars1 InsertScalar 1  1.0 
    scalars1 InsertScalar 2  2.0 

  vtkCellArray ca1
    ca1 InsertNextCell 3 
    ca1 InsertCellPoint 0 
    ca1 InsertCellPoint 1 
    ca1 InsertCellPoint 2 

  vtkPolyData line1
    line1 SetPoints pts1 
    line1 SetLines ca1 
    [line1 GetPointData] SetScalars scalars1 

  vtkPolyDataMapper line1Map
    line1Map SetInput line1 
    line1Map ScalarVisibilityOn  
    line1Map ImmediateModeRenderingOn  
    line1Map SetScalarRange 0.0 2.0 
    # Note: the combo of ImmediateModeRenderingOn + SetLookupTable is a bug
    line1Map SetLookupTable lut 

  vtkActor line1Actor
    line1Actor SetMapper line1Map 
    [line1Actor GetProperty] SetLineWidth 10

  # ----- create line #2 ---------
  vtkPoints pts2
    pts2 InsertPoint 0  -1 1 0 
    pts2 InsertPoint 1   0 1 0 
    pts2 InsertPoint 2   1 1 0 

  vtkScalars scalars2
    scalars2 InsertScalar 0  0.0 
    scalars2 InsertScalar 1  1.0 
    scalars2 InsertScalar 2  2.0 

  vtkCellArray ca2
    ca2 InsertNextCell 3 
    ca2 InsertCellPoint 0 
    ca2 InsertCellPoint 1 
    ca2 InsertCellPoint 2 

  vtkPolyData line2
    line2 SetPoints pts2 
    line2 SetLines ca2 
    [line2 GetPointData] SetScalars scalars2 

  vtkPolyDataMapper line2Map
    line2Map SetInput line2 
    line2Map ScalarVisibilityOn  
    line2Map ImmediateModeRenderingOn  
    line2Map SetScalarRange 0.0 1.0 
    # Note: the combo of ImmediateModeRenderingOn + SetLookupTable is a bug
    line2Map SetLookupTable lut 

  vtkActor line2Actor
    line2Actor SetMapper line2Map 
    [line2Actor GetProperty] SetLineWidth 10

  # ----- Add the actors ---------
  # Note: the actor that's added last gets its LUT used
  ren1 AddActor line1Actor 
  ren1 AddActor line2Actor 

  ren1 SetBackground 0 0 0 
  renWin SetSize 300 300 
iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

  renWin Render  
# prevent the tk window from showing up then start the event loop
wm withdraw .
