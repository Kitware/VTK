# Demonstrate clipping on single hexahedron.
# See orderedTriangulation.tcl to see how point order controls triangulation
catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# Setup: interactor and colors
wm withdraw .
source $VTK_TCL/vtkInt.tcl
source $VTK_TCL/colors.tcl

set ifoo(0) 0
set ifoo(1) 1
set ifoo(2) 2
set ifoo(3) 3
set ifoo(4) 4
set ifoo(5) 5
set ifoo(6) 6
set ifoo(7) 7

# Define a Single Cube. Everything is inside.
#
vtkScalars Scalars
    Scalars InsertNextScalar 1.0
    Scalars InsertNextScalar 1.0
    Scalars InsertNextScalar 1.0
    Scalars InsertNextScalar 1.0
    Scalars InsertNextScalar 1.0
    Scalars InsertNextScalar 1.0
    Scalars InsertNextScalar 1.0
    Scalars InsertNextScalar 1.0

vtkPoints Points
    Points InsertNextPoint 0 0 0
    Points InsertNextPoint 1 0 0
    Points InsertNextPoint 1 1 0
    Points InsertNextPoint 0 1 0
    Points InsertNextPoint 0 0 1
    Points InsertNextPoint 1 0 1
    Points InsertNextPoint 1 1 1
    Points InsertNextPoint 0 1 1

vtkIdList Ids
vtkUnstructuredGrid Grid

# Procedure to control order of triangulation
#
proc tri {i1 i2 i3 i4 i5 i6 i7 i8} {
    global ifoo

    set ii(0) $i1
    set ii(1) $i2
    set ii(2) $i3
    set ii(3) $i4
    set ii(4) $i5
    set ii(5) $i6
    set ii(6) $i7
    set ii(7) $i8

    # Error check
    for {set j 0} {$j < 8} {incr j} {
        if { $ii($j) < 0 } {
            tk_messageBox -message "Index <= 0"
            return
        }
        if { $ii($j) > 7 } {
            tk_messageBox -message "Index > 7"
            return
        }
        for {set jj 0} {$jj < 7} {incr jj} {
            for {set jjj [expr $jj + 1]} {$jjj < 8} {incr jjj} {
                if { $ii($jj) == $ii($jjj) } {
                    tk_messageBox -message "Equal indices"
                    return
                }
            }
        }
    }

    # Okay, everything consistent
    for {set j 0} {$j < 8} {incr j} {
        set i($j) $ii($j)
    }
    
    Ids InsertNextId $ifoo(0)
    Ids InsertNextId $ifoo(1)
    Ids InsertNextId $ifoo(2)
    Ids InsertNextId $ifoo(3)
    Ids InsertNextId $ifoo(4)
    Ids InsertNextId $ifoo(5)
    Ids InsertNextId $ifoo(6)
    Ids InsertNextId $ifoo(7)

    Grid Initialize
    Grid Allocate 10 10
    Grid InsertNextCell 12 Ids
    Grid SetPoints Points
    [Grid GetPointData] SetScalars Scalars

    #Update the annotation
    caseLabel SetText "Order: $ifoo(0) $ifoo(1) $ifoo(2) $ifoo(3)\
            $ifoo(4) $ifoo(5) $ifoo(6) $ifoo(7)"

    Grid Modified
    renWin Render
}

# Clip the hex. The side effect is an ordered triangulation.
#
vtkClipDataSet clipper
    clipper SetInput Grid
    clipper SetValue 0.5

# Build tubes for the edges of the tets.
#
vtkExtractEdges tetEdges
    tetEdges SetInput [clipper GetOutput]
vtkTubeFilter tetEdgeTubes
    tetEdgeTubes SetInput [tetEdges GetOutput]
    tetEdgeTubes SetRadius .01
    tetEdgeTubes SetNumberOfSides 6
vtkPolyDataMapper tetEdgeMapper
    tetEdgeMapper SetInput [tetEdgeTubes GetOutput]
    tetEdgeMapper ScalarVisibilityOff
vtkActor tetEdgeActor
    tetEdgeActor SetMapper tetEdgeMapper
    eval [tetEdgeActor GetProperty] SetDiffuseColor $khaki
    [tetEdgeActor GetProperty] SetSpecular .4
    [tetEdgeActor GetProperty] SetSpecularPower 10

# Shrink the tets. This disconnects them so that you can
# see all the edges, even the interior ones.
vtkShrinkFilter aShrinker
    aShrinker SetShrinkFactor 1
    aShrinker SetInput [clipper GetOutput]
vtkDataSetMapper aMapper
    aMapper ScalarVisibilityOff
    aMapper SetInput [aShrinker GetOutput]
vtkActor Tets
    Tets SetMapper aMapper
    eval [Tets GetProperty] SetDiffuseColor $banana

# Build the vertices of the cube.
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

# Define the text for the label. It shows the order of insertion.
#
vtkVectorText caseLabel
    caseLabel SetText "Order: "

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

# Create the point labels
vtkCamera camera
vtkVectorText label0
    label0 SetText "0"
vtkPolyDataMapper label0Mapper
    label0Mapper SetInput [label0 GetOutput]
vtkFollower label0Actor
    label0Actor SetMapper label0Mapper
    label0Actor SetCamera camera
    label0Actor SetScale 0.05 0.05 0.05
    label0Actor AddPosition -0.1 0.0 -0.1

vtkVectorText label1
    label1 SetText "1"
vtkPolyDataMapper label1Mapper
    label1Mapper SetInput [label1 GetOutput]
vtkFollower label1Actor
    label1Actor SetMapper label1Mapper
    label1Actor SetCamera camera
    label1Actor SetScale 0.05 0.05 0.05
    label1Actor AddPosition 1.1 0.0 -0.1

vtkVectorText label2
    label2 SetText "2"
vtkPolyDataMapper label2Mapper
    label2Mapper SetInput [label2 GetOutput]
vtkFollower label2Actor
    label2Actor SetMapper label2Mapper
    label2Actor SetCamera camera
    label2Actor SetScale 0.05 0.05 0.05
    label2Actor AddPosition 1.1 1.1 -0.1

vtkVectorText label3
    label3 SetText "3"
vtkPolyDataMapper label3Mapper
    label3Mapper SetInput [label3 GetOutput]
vtkFollower label3Actor
    label3Actor SetMapper label3Mapper
    label3Actor SetCamera camera
    label3Actor SetScale 0.05 0.05 0.05
    label3Actor AddPosition -0.1 1.1 -0.1

vtkVectorText label4
    label4 SetText "4"
vtkPolyDataMapper label4Mapper
    label4Mapper SetInput [label4 GetOutput]
vtkFollower label4Actor
    label4Actor SetMapper label4Mapper
    label4Actor SetCamera camera
    label4Actor SetScale 0.05 0.05 0.05
    label4Actor AddPosition -0.1 0.0 1.1

vtkVectorText label5
    label5 SetText "5"
vtkPolyDataMapper label5Mapper
    label5Mapper SetInput [label5 GetOutput]
vtkFollower label5Actor
    label5Actor SetMapper label5Mapper
    label5Actor SetCamera camera
    label5Actor SetScale 0.05 0.05 0.05
    label5Actor AddPosition 1.1 0.0 1.1

vtkVectorText label6
    label6 SetText "6"
vtkPolyDataMapper label6Mapper
    label6Mapper SetInput [label6 GetOutput]
vtkFollower label6Actor
    label6Actor SetMapper label6Mapper
    label6Actor SetCamera camera
    label6Actor SetScale 0.05 0.05 0.05
    label6Actor AddPosition 1.1 1.1 1.1

vtkVectorText label7
    label7 SetText "7"
vtkPolyDataMapper label7Mapper
    label7Mapper SetInput [label7 GetOutput]
vtkFollower label7Actor
    label7Actor SetMapper label7Mapper
    label7Actor SetCamera camera
    label7Actor SetScale 0.05 0.05 0.05
    label7Actor AddPosition -0.1 1.1 1.1

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
    ren1 SetActiveCamera camera
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# position the base
base SetPosition .5 -.09 .5

ren1 AddActor Tets
ren1 AddActor tetEdgeActor
ren1 AddActor base
ren1 AddActor CubeVertices
ren1 AddActor labelActor
ren1 AddActor label0Actor
ren1 AddActor label1Actor
ren1 AddActor label2Actor
ren1 AddActor label3Actor
ren1 AddActor label4Actor
ren1 AddActor label5Actor
ren1 AddActor label6Actor
ren1 AddActor label7Actor

eval ren1 SetBackground $slate_grey
iren SetUserMethod {wm deiconify .vtkInteract}

# Perform the ordered triangulation
renWin SetSize 350 350
tri 5 7 2 1 6 4 3 0
ren1 ResetCamera

[ren1 GetActiveCamera] Dolly 1.2
[ren1 GetActiveCamera] Azimuth 30
[ren1 GetActiveCamera] Elevation 20
ren1 ResetCameraClippingRange

iren Initialize



