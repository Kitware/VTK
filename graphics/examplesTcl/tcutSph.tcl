#
# cut an outer sphere to reveal an inner sphere
#
# converted from tcutSph.cxx

catch {load vtktcl}
# get the interactor ui
source ../../examplesTcl/vtkInt.tcl
source ../../examplesTcl/colors.tcl

# Create the RenderWindow  Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# hidden sphere
vtkSphereSource sphere1
  sphere1 SetThetaResolution 12 
  sphere1 SetPhiResolution 12 
  sphere1 SetRadius 0.5 

vtkPolyDataMapper innerMapper
innerMapper SetInput [sphere1 GetOutput ]

vtkActor innerSphere
  innerSphere SetMapper innerMapper 
[innerSphere GetProperty] SetColor  1  .9216  .8039

# sphere to texture
vtkSphereSource sphere2
  sphere2 SetThetaResolution 24 
  sphere2 SetPhiResolution 24 
  sphere2 SetRadius 1.0 

vtkPoints points
  points InsertPoint  0  0 0 0
  points InsertPoint  1  0 0 0

vtkNormals normals
  normals InsertNormal  0  1 0 0
  normals InsertNormal  1  0 1 0

vtkPlanes planes
  planes SetPoints  points 
  planes SetNormals  normals 

vtkImplicitTextureCoords tcoords
tcoords SetInput [sphere2 GetOutput ]
  tcoords SetRFunction planes 

vtkDataSetMapper outerMapper
outerMapper SetInput [tcoords GetOutput]

vtkStructuredPointsReader tmap
  tmap SetFileName "../../../vtkdata/texThres.vtk" 

vtkTexture texture
  texture SetInput [tmap GetOutput]
  texture InterpolateOff
  texture RepeatOff 

vtkActor outerSphere
  outerSphere SetMapper outerMapper 
  outerSphere SetTexture texture 
  [outerSphere GetProperty] SetColor  1  .6275  .4784 

ren1 AddActor innerSphere 
ren1 AddActor outerSphere 
ren1 SetBackground  0.4392 0.5020 0.5647 
renWin SetSize  500 500 

# interact with data
renWin Render
iren Initialize
iren SetUserMethod {wm deiconify .vtkInteract}

#renWin SetFileName "tcutSph.tcl.ppm"
#renWin SaveImageAsPPM

wm withdraw .
 