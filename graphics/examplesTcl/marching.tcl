# include get the vtk interactor ui
catch {load vtktcl}
# create camera figure
source ../../examplesTcl/vtkInt.tcl
source ../../examplesTcl/vtkInclude.tcl

# get some good color definitions
source ../../examplesTcl/colors.tcl
# get the procs that define the marching cubes cases
source mccases.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

#define a Single Cube
vtkFloatScalars Scalars
	Scalars InsertNextScalar 1.0
	Scalars InsertNextScalar 0.0
	Scalars InsertNextScalar 0.0
	Scalars InsertNextScalar 1.0
	Scalars InsertNextScalar 0.0
	Scalars InsertNextScalar 0.0
	Scalars InsertNextScalar 0.0
	Scalars InsertNextScalar 0.0

vtkFloatPoints Points
        Points InsertNextPoint 0 0 0
        Points InsertNextPoint 1 0 0
        Points InsertNextPoint 1 1 0
        Points InsertNextPoint 0 1 0
        Points InsertNextPoint 0 0 1
        Points InsertNextPoint 1 0 1
        Points InsertNextPoint 1 1 1
        Points InsertNextPoint 0 1 1

vtkIdList Ids
	Ids InsertNextId 0
	Ids InsertNextId 1
	Ids InsertNextId 2
	Ids InsertNextId 3
	Ids InsertNextId 4
	Ids InsertNextId 5
	Ids InsertNextId 6
	Ids InsertNextId 7

vtkUnstructuredGrid Grid
        Grid Allocate 10 10
	Grid InsertNextCell 12 Ids
        Grid SetPoints Points
        [Grid GetPointData] SetScalars Scalars

#extract the triangles for the cube

vtkContourFilter Marching
	Marching SetInput Grid
        Marching SetValue 0 0.5
	Marching Update

#
# build tubes for the triangle edges
#
vtkExtractEdges triangleEdges
	triangleEdges SetInput [Marching GetOutput]
vtkTubeFilter triangleEdgeTubes
	triangleEdgeTubes SetInput [triangleEdges GetOutput]
	triangleEdgeTubes SetRadius .005
	triangleEdgeTubes SetNumberOfSides 6
	triangleEdgeTubes UseDefaultNormalOn
	triangleEdgeTubes SetDefaultNormal .577 .577 .577
vtkPolyDataMapper triangleEdgeMapper
	triangleEdgeMapper SetInput [triangleEdgeTubes GetOutput]
	triangleEdgeMapper ScalarVisibilityOff
vtkActor triangleEdgeActor
	triangleEdgeActor SetMapper triangleEdgeMapper
	eval [triangleEdgeActor GetProperty] SetDiffuseColor $lamp_black
	[triangleEdgeActor GetProperty] SetSpecular .4
	[triangleEdgeActor GetProperty] SetSpecularPower 10
ren1 AddActor triangleEdgeActor
#shrink the triangles so we can see each one
vtkShrinkPolyData aShrinker
	aShrinker SetShrinkFactor 1
	aShrinker SetInput [Marching GetOutput]
vtkPolyDataMapper aMapper
	aMapper ScalarVisibilityOff
	aMapper SetInput [aShrinker GetOutput]
vtkActor Triangles
	Triangles SetMapper aMapper
	eval [Triangles GetProperty] SetDiffuseColor $banana
[Triangles GetProperty] SetOpacity .6

#build a model of the cube
vtkCubeSource CubeModel
	CubeModel SetCenter .5 .5 .5
vtkExtractEdges Edges
	Edges SetInput [CubeModel GetOutput]
vtkTubeFilter Tubes
	Tubes SetInput [Edges GetOutput]
	Tubes SetRadius .01
	Tubes SetNumberOfSides 6
	Tubes UseDefaultNormalOn
	Tubes SetDefaultNormal .577 .577 .577
vtkPolyDataMapper TubeMapper
	TubeMapper SetInput [Tubes GetOutput]
vtkActor CubeEdges
	CubeEdges SetMapper TubeMapper
	eval [CubeEdges GetProperty] SetDiffuseColor $khaki
	[CubeEdges GetProperty] SetSpecular .4
	[CubeEdges GetProperty] SetSpecularPower 10

#
# build the vertices of the cube
#
vtkSphereSource Sphere
  Sphere SetRadius 0.04
  Sphere SetPhiResolution 20
  Sphere SetThetaResolution 20
vtkThresholdPoints ThresholdIn
  ThresholdIn SetInput Grid
  ThresholdIn ThresholdByUpper .5
vtkGlyph3D Vertices
  Vertices SetInput [ThresholdIn GetOutput]
  Vertices SetSource [Sphere GetOutput]
vtkPolyDataMapper SphereMapper
  SphereMapper SetInput [Vertices GetOutput]
  SphereMapper ScalarVisibilityOff
vtkActor CubeVertices
  CubeVertices SetMapper SphereMapper
  eval [CubeVertices GetProperty] SetDiffuseColor $tomato
  eval [CubeVertices GetProperty] SetDiffuseColor $tomato

#define the text for the labels
vtkVectorText caseLabel
  caseLabel SetText "Case 1"

vtkTransform aLabelTransform
  aLabelTransform Identity
  aLabelTransform Translate  -.2 0 1.25
  aLabelTransform Scale .05 .05 .05

vtkTransformPolyDataFilter labelTransform
  labelTransform SetTransform aLabelTransform
  labelTransform SetInput [caseLabel GetOutput]
  
 
vtkPolyDataMapper labelMapper
  labelMapper SetInput [labelTransform GetOutput];
 
vtkActor labelActor
  labelActor SetMapper labelMapper
 
#define the base 
vtkCubeSource baseModel
  baseModel SetXLength 1.5
  baseModel SetYLength .01
  baseModel SetZLength 1.5
vtkPolyDataMapper baseMapper
  baseMapper SetInput [baseModel GetOutput]
vtkActor base
  base SetMapper baseMapper

#
# position the base
base SetPosition .5 -.09 .5


ren1 AddActor base
ren1 AddActor labelActor
ren1 AddActor CubeEdges
ren1 AddActor CubeVertices
ren1 AddActor Triangles
eval ren1 SetBackground $slate_grey
iren SetUserMethod {wm deiconify .vtkInteract}
case12 Scalars 0 1
Grid Modified
renWin SetStereoType 1
renWin SetSize 640 480

[ren1 GetActiveCamera] Dolly 1.2
[ren1 GetActiveCamera] Azimuth 30
[ren1 GetActiveCamera] Elevation 20
renWin Render
iren Initialize

#
# get the user interface to select the cases
# if we are not running regression tests
#
if { [info command rtExMath] == "" } {
  source mccasesui.tcl
} else {
  wm withdraw .
}
#renWin SetFileName marching.tcl.ppm
#renWin SaveImageAsPPM
