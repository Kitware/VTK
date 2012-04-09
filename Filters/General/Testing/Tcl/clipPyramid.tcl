package require vtk
package require vtkinteraction
package require vtktesting

#define a Single Cube
vtkFloatArray Scalars
    Scalars InsertNextValue 1.0
    Scalars InsertNextValue 0.0
    Scalars InsertNextValue 0.0
    Scalars InsertNextValue 0.0
    Scalars InsertNextValue 0.0
    Scalars InsertNextValue 0.0

vtkPoints Points
    Points InsertNextPoint 0 0 0
    Points InsertNextPoint 1 0 0
    Points InsertNextPoint 1 1 0
    Points InsertNextPoint 0 1 0
    Points InsertNextPoint .5 .5 1

vtkIdList Ids
    Ids InsertNextId 0
    Ids InsertNextId 1
    Ids InsertNextId 2
    Ids InsertNextId 3
    Ids InsertNextId 4

vtkUnstructuredGrid Grid
    Grid Allocate 10 10
    Grid InsertNextCell 14 Ids
    Grid SetPoints Points
    [Grid GetPointData] SetScalars Scalars

#Clip the pyramid
vtkClipDataSet clipper
    clipper SetInputData Grid
    clipper SetValue 0.5

# build tubes for the triangle edges
#
vtkExtractEdges pyrEdges
    pyrEdges SetInputConnection [clipper GetOutputPort]
vtkTubeFilter pyrEdgeTubes
    pyrEdgeTubes SetInputConnection [pyrEdges GetOutputPort]
    pyrEdgeTubes SetRadius .005
    pyrEdgeTubes SetNumberOfSides 6
vtkPolyDataMapper pyrEdgeMapper
    pyrEdgeMapper SetInputConnection [pyrEdgeTubes GetOutputPort]
    pyrEdgeMapper ScalarVisibilityOff
vtkActor pyrEdgeActor
    pyrEdgeActor SetMapper pyrEdgeMapper
    eval [pyrEdgeActor GetProperty] SetDiffuseColor $lamp_black
    [pyrEdgeActor GetProperty] SetSpecular .4
    [pyrEdgeActor GetProperty] SetSpecularPower 10

#shrink the triangles so we can see each one
vtkShrinkFilter aShrinker
    aShrinker SetShrinkFactor 1
    aShrinker SetInputConnection [clipper GetOutputPort]
vtkDataSetMapper aMapper
    aMapper ScalarVisibilityOff
    aMapper SetInputConnection [aShrinker GetOutputPort]
vtkActor Pyrs
    Pyrs SetMapper aMapper
    eval [Pyrs GetProperty] SetDiffuseColor $banana

#build a model of the pyramid
vtkExtractEdges Edges
    Edges SetInputData Grid
vtkTubeFilter Tubes
    Tubes SetInputConnection [Edges GetOutputPort]
    Tubes SetRadius .01
    Tubes SetNumberOfSides 6
vtkPolyDataMapper TubeMapper
    TubeMapper SetInputConnection [Tubes GetOutputPort]
    TubeMapper ScalarVisibilityOff
vtkActor CubeEdges
    CubeEdges SetMapper TubeMapper
    eval [CubeEdges GetProperty] SetDiffuseColor $khaki
    [CubeEdges GetProperty] SetSpecular .4
    [CubeEdges GetProperty] SetSpecularPower 10

# build the vertices of the pyramid
#
vtkSphereSource Sphere
    Sphere SetRadius 0.04
    Sphere SetPhiResolution 20
    Sphere SetThetaResolution 20
vtkThresholdPoints ThresholdIn
    ThresholdIn SetInputData Grid
    ThresholdIn ThresholdByUpper .5
vtkGlyph3D Vertices
    Vertices SetInputConnection [ThresholdIn GetOutputPort]
    Vertices SetSourceConnection [Sphere GetOutputPort]
vtkPolyDataMapper SphereMapper
    SphereMapper SetInputConnection [Vertices GetOutputPort]
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
    labelTransform SetInputConnection [caseLabel GetOutputPort]

vtkPolyDataMapper labelMapper
    labelMapper SetInputConnection [labelTransform GetOutputPort];

vtkActor labelActor
    labelActor SetMapper labelMapper

#define the base
vtkCubeSource baseModel
    baseModel SetXLength 1.5
    baseModel SetYLength .01
    baseModel SetZLength 1.5
vtkPolyDataMapper baseMapper
    baseMapper SetInputConnection [baseModel GetOutputPort]
vtkActor base
    base SetMapper baseMapper

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# position the base
base SetPosition .5 -.09 .5

ren1 AddActor pyrEdgeActor
ren1 AddActor base
ren1 AddActor labelActor
ren1 AddActor CubeEdges
ren1 AddActor CubeVertices
ren1 AddActor Pyrs
eval ren1 SetBackground $slate_grey
iren AddObserver UserEvent {wm deiconify .vtkInteract}

renWin SetSize 400 400

ren1 ResetCamera
[ren1 GetActiveCamera] Dolly 1.3
[ren1 GetActiveCamera] Elevation 15
ren1 ResetCameraClippingRange

renWin Render
iren Initialize

set mask "1 2 4 8 16 32"
proc cases {id} {
    global mask
    for {set i 0} {$i < 5} {incr i} {
	set m [lindex $mask $i]
	if {[expr $m & $id] == 0} {
	    Scalars SetValue $i 0
	} else {
	    Scalars SetValue $i 1
	}
    caseLabel SetText "Case $id"
    }
    Grid Modified
    renWin Render
}

cases 20

wm withdraw .

