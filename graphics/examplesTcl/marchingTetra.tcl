# include get the vtk interactor ui
catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# create camera figure
source $VTK_TCL/vtkInt.tcl
source $VTK_TCL/vtkInclude.tcl

# get some good color definitions
source $VTK_TCL/colors.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

#define a Single Tetra
vtkScalars Scalars
	Scalars InsertNextScalar 1.0
	Scalars InsertNextScalar 0.0
	Scalars InsertNextScalar 0.0
	Scalars InsertNextScalar 0.0

vtkPoints Points
        Points InsertNextPoint 0 0 0
        Points InsertNextPoint 1 0 0
        Points InsertNextPoint 0 0 1
        Points InsertNextPoint 1 1 1

vtkIdList Ids
	Ids InsertNextId 0
	Ids InsertNextId 1
	Ids InsertNextId 2
	Ids InsertNextId 3

vtkUnstructuredGrid Grid
        Grid Allocate 10 10
	Grid InsertNextCell 10 Ids
        Grid SetPoints Points
        [Grid GetPointData] SetScalars Scalars

#extract the triangles for the tetra

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
	triangleEdgeTubes SetDefaultNormal .1 .2 .3
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

vtkProperty backProp
eval   backProp SetDiffuseColor $tomato
backProp SetOpacity .6

Triangles SetBackfaceProperty backProp

#build a model of the cube
vtkExtractEdges Edges
	Edges SetInput Grid
vtkTubeFilter Tubes
	Tubes SetInput [Edges GetOutput]
	Tubes SetRadius .01
	Tubes SetNumberOfSides 6
vtkPolyDataMapper TubeMapper
	TubeMapper SetInput [Tubes GetOutput]
        TubeMapper ScalarVisibilityOff
vtkActor tetraEdges
	tetraEdges SetMapper TubeMapper
	eval [tetraEdges GetProperty] SetDiffuseColor $khaki
	[tetraEdges GetProperty] SetSpecular .4
	[tetraEdges GetProperty] SetSpecularPower 10

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
  Vertices SetScaleModeToDataScalingOff
vtkPolyDataMapper SphereMapper
  SphereMapper SetInput [Vertices GetOutput]
  SphereMapper ScalarVisibilityOff
vtkActor tetraVertices
  tetraVertices SetMapper SphereMapper
  eval [tetraVertices GetProperty] SetDiffuseColor $tomato
  eval [tetraVertices GetProperty] SetDiffuseColor $tomato

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
ren1 AddActor tetraEdges
ren1 AddActor tetraVertices
ren1 AddActor Triangles
eval ren1 SetBackground $slate_grey
iren SetUserMethod {wm deiconify .vtkInteract}
Grid Modified
renWin SetStereoType 1
renWin SetSize 640 480

[ren1 GetActiveCamera] Dolly 1.2
[ren1 GetActiveCamera] Azimuth 30
[ren1 GetActiveCamera] Elevation 20
ren1 ResetCameraClippingRange

renWin Render
iren Initialize

set mask {1 2 4 8 16 32}
proc cases {id} {
    global mask
    for {set i 0} {$i < 4} {incr i} {
	set m [lindex $mask $i]
	if {[expr $m & $id] == 0} {
	    Scalars SetScalar $i 0
	} else {
	    Scalars SetScalar $i 1
	}
    caseLabel SetText "Case $id"
    }
Marching Modified
ThresholdIn Modified
renWin Render
}
cases 1
    
wm withdraw .

