catch {load vtktcl}
# user interface command widget
source vtkInt.tcl

# create a rendering window and renderer
vtkRenderMaster rm
set renWin [rm MakeRenderWindow]
set ren1 [$renWin MakeRenderer]
set iren [$renWin MakeRenderWindowInteractor]

vtkConeSource cube
#vtkPlaneSource cube
#  cube SetResolution 5 5
#vtkCubeSource cube
vtkCleanPolyData clean;#remove duplicate vertices and edges
  clean SetInput [cube GetOutput]
vtkExtractEdges extract
  extract SetInput [clean GetOutput]
vtkTubeFilter tubes
  tubes SetInput [extract GetOutput]
  tubes SetRadius 0.05
  tubes SetNumberOfSides 6
vtkPolyMapper mapper
  mapper SetInput [tubes GetOutput]
vtkActor cubeActor
  cubeActor SetMapper mapper

vtkSphereSource sphere
  sphere SetRadius 0.080
vtkGlyph3D verts
  verts SetInput [cube GetOutput]
  verts SetSource [sphere GetOutput]
vtkPolyMapper sphereMapper
  sphereMapper SetInput [verts GetOutput]
vtkActor vertActor
  vertActor SetMapper sphereMapper
  [vertActor GetProperty] SetColor 0 0 1

# assign our actor to the renderer
$ren1 AddActors cubeActor
$ren1 AddActors vertActor

# enable user interface interactor
$iren SetUserMethod {wm deiconify .vtkInteract}
$iren Initialize

#$renWin SetFileName "cubeEdges.tcl.ppm"
#$renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .

