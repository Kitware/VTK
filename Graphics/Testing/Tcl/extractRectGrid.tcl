package require vtk
package require vtkinteraction

# create pipeline - rectilinear grid
#
vtkRectilinearGridReader rgridReader
    rgridReader SetFileName "$VTK_DATA_ROOT/Data/RectGrid2.vtk"
vtkOutlineFilter outline
  outline SetInput [rgridReader GetOutput]
vtkPolyDataMapper mapper
  mapper SetInput [outline GetOutput]
vtkActor actor
  actor SetMapper mapper

rgridReader Update

vtkExtractRectilinearGrid extract1
   extract1 SetInput [rgridReader GetOutput]
   #extract1 SetVOI 0 46 0 32 0 10
   extract1 SetVOI 23 40 16 30 9 9
   extract1 SetSampleRate 2 2 1
   extract1 IncludeBoundaryOn
   extract1 Update
vtkDataSetSurfaceFilter surf1
    surf1 SetInput [extract1 GetOutput]
vtkTriangleFilter tris
    tris SetInput [surf1 GetOutput]
vtkPolyDataMapper mapper1
    mapper1 SetInput [tris GetOutput]
    eval mapper1 SetScalarRange [[extract1 GetOutput] GetScalarRange]
vtkActor actor1
    actor1 SetMapper mapper1

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


