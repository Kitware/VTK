package require vtk
package require vtkinteraction

# create pipeline - rectilinear grid
#
vtkRectilinearGridReader rgridReader
    rgridReader SetFileName "$VTK_DATA_ROOT/Data/RectGrid2.vtk"
vtkOutlineFilter outline
  outline SetInputConnection [rgridReader GetOutputPort]
vtkPolyDataMapper mapper
  mapper SetInputConnection [outline GetOutputPort]
vtkActor actor
  actor SetMapper mapper

rgridReader Update

vtkExtractRectilinearGrid extract1
   extract1 SetInputConnection [rgridReader GetOutputPort]
   #extract1 SetVOI 0 46 0 32 0 10
   extract1 SetVOI 23 40 16 30 9 9
   extract1 SetSampleRate 2 2 1
   extract1 IncludeBoundaryOn
   extract1 Update
vtkDataSetSurfaceFilter surf1
    surf1 SetInputConnection [extract1 GetOutputPort]
vtkTriangleFilter tris
    tris SetInputConnection [surf1 GetOutputPort]
vtkPolyDataMapper mapper1
    mapper1 SetInputConnection [tris GetOutputPort]
    eval mapper1 SetScalarRange [[extract1 GetOutput] GetScalarRange]
vtkActor actor1
    actor1 SetMapper mapper1

# write out a rect grid
# write to the temp directory if possible, otherwise use .
set dir "."
if {[info commands "rtTester"] == "rtTester"}  {
   set dir [rtTester GetTempDirectory]
}

# make sure the directory is writeable first
if {[catch {set channel [open "$dir/test.tmp" "w"]}] == 0 } {
   close $channel
   file delete -force "$dir/test.tmp"
   
   vtkRectilinearGridWriter rectWriter
   rectWriter SetInputConnection [extract1 GetOutputPort]
   rectWriter SetFileName "$dir/rect.tmp"
   rectWriter Write   
   # delete the file
   file delete -force "$dir/rect.tmp"
}


# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

#ren1 AddActor actor
ren1 AddActor actor1
renWin SetSize 340 400

iren Initialize

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .


