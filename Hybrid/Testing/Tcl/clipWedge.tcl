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
    Points InsertNextPoint 0 0 1
    Points InsertNextPoint 0 1 0
    Points InsertNextPoint 1 1 0
    Points InsertNextPoint 0 1 1

vtkIdList Ids
    Ids InsertNextId 0
    Ids InsertNextId 1
    Ids InsertNextId 2
    Ids InsertNextId 3
    Ids InsertNextId 4
    Ids InsertNextId 5

vtkUnstructuredGrid Grid
    Grid Allocate 10 10
    Grid InsertNextCell 13 Ids
    Grid SetPoints Points
    [Grid GetPointData] SetScalars Scalars

#Clip the wedge
vtkClipDataSet clipper
    clipper SetInput Grid
    clipper SetValue 0.5

# build tubes for the triangle edges
#
vtkExtractEdges wedgeEdges
    wedgeEdges SetInput [clipper GetOutput]
vtkTubeFilter wedgeEdgeTubes
    wedgeEdgeTubes SetInput [wedgeEdges GetOutput]
    wedgeEdgeTubes SetRadius .005
    wedgeEdgeTubes SetNumberOfSides 6
vtkPolyDataMapper wedgeEdgeMapper
    wedgeEdgeMapper SetInput [wedgeEdgeTubes GetOutput]
    wedgeEdgeMapper ScalarVisibilityOff
vtkActor wedgeEdgeActor
    wedgeEdgeActor SetMapper wedgeEdgeMapper
    eval [wedgeEdgeActor GetProperty] SetDiffuseColor $lamp_black
    [wedgeEdgeActor GetProperty] SetSpecular .4
    [wedgeEdgeActor GetProperty] SetSpecularPower 10

#shrink the triangles so we can see each one
vtkShrinkFilter aShrinker
    aShrinker SetShrinkFactor 1
    aShrinker SetInput [clipper GetOutput]
vtkDataSetMapper aMapper
    aMapper ScalarVisibilityOff
    aMapper SetInput [aShrinker GetOutput]
vtkActor Wedges
    Wedges SetMapper aMapper
    eval [Wedges GetProperty] SetDiffuseColor $banana

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
vtkActor CubeEdges
    CubeEdges SetMapper TubeMapper
    eval [CubeEdges GetProperty] SetDiffuseColor $khaki
    [CubeEdges GetProperty] SetSpecular .4
    [CubeEdges GetProperty] SetSpecularPower 10

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

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# position the base
base SetPosition .5 -.09 .5

ren1 AddActor wedgeEdgeActor
ren1 AddActor base
ren1 AddActor labelActor
ren1 AddActor CubeEdges
ren1 AddActor CubeVertices
ren1 AddActor Wedges
eval ren1 SetBackground $slate_grey
iren AddObserver UserEvent {wm deiconify .vtkInteract}

renWin SetSize 400 400

[ren1 GetActiveCamera] Dolly 1.2
[ren1 GetActiveCamera] Azimuth 30
[ren1 GetActiveCamera] Elevation 20
ren1 ResetCameraClippingRange

renWin Render
iren Initialize

set mask {1 2 4 8 16 32}
proc cases {id} {
    global mask
    for {set i 0} {$i < 6} {incr i} {
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

cases 7
    
wm withdraw .

