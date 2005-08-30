# This example demonstrates the use of the vtkTransformPolyDataFilter
# to reposition a 3D text string.

#
# First we include the VTK Tcl packages which will make available
# all of the vtk commands to Tcl
#
package require vtk
package require vtkinteraction
package require vtktesting

#define a Single Cube
vtkFloatArray Scalars
	Scalars InsertNextValue 1.0
	Scalars InsertNextValue 0.0
	Scalars InsertNextValue 0.0
	Scalars InsertNextValue 1.0
	Scalars InsertNextValue 0.0
	Scalars InsertNextValue 0.0
	Scalars InsertNextValue 0.0
	Scalars InsertNextValue 0.0

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

# Find the triangles that lie along the 0.5 contour in this cube.
vtkContourFilter Marching
	Marching SetInput Grid
        Marching SetValue 0 0.5
	Marching Update

# Extract the edges of the triangles just found.
vtkExtractEdges triangleEdges
	triangleEdges SetInputConnection [Marching GetOutputPort]
# Draw the edges as tubes instead of lines.  Also create the associated
# mapper and actor to display the tubes.
vtkTubeFilter triangleEdgeTubes
	triangleEdgeTubes SetInputConnection [triangleEdges GetOutputPort]
	triangleEdgeTubes SetRadius .005
	triangleEdgeTubes SetNumberOfSides 6
	triangleEdgeTubes UseDefaultNormalOn
	triangleEdgeTubes SetDefaultNormal .577 .577 .577
vtkPolyDataMapper triangleEdgeMapper
	triangleEdgeMapper SetInputConnection [triangleEdgeTubes GetOutputPort]
	triangleEdgeMapper ScalarVisibilityOff
vtkActor triangleEdgeActor
	triangleEdgeActor SetMapper triangleEdgeMapper
	eval [triangleEdgeActor GetProperty] SetDiffuseColor $lamp_black
	[triangleEdgeActor GetProperty] SetSpecular .4
	[triangleEdgeActor GetProperty] SetSpecularPower 10

# Shrink the triangles we found earlier.  Create the associated mapper
# and actor.  Set the opacity of the shrunken triangles.
vtkShrinkPolyData aShrinker
	aShrinker SetShrinkFactor 1
	aShrinker SetInputConnection [Marching GetOutputPort]
vtkPolyDataMapper aMapper
	aMapper ScalarVisibilityOff
	aMapper SetInputConnection [aShrinker GetOutputPort]
vtkActor Triangles
	Triangles SetMapper aMapper
	eval [Triangles GetProperty] SetDiffuseColor $banana
[Triangles GetProperty] SetOpacity .6

# Draw a cube the same size and at the same position as the one created
# previously.  Extract the edges because we only want to see the outline
# of the cube.  Pass the edges through a vtkTubeFilter so they are displayed
# as tubes rather than lines.
vtkCubeSource CubeModel
	CubeModel SetCenter .5 .5 .5
vtkExtractEdges Edges
	Edges SetInputConnection [CubeModel GetOutputPort]
vtkTubeFilter Tubes
	Tubes SetInputConnection [Edges GetOutputPort]
	Tubes SetRadius .01
	Tubes SetNumberOfSides 6
	Tubes UseDefaultNormalOn
	Tubes SetDefaultNormal .577 .577 .577
# Create the mapper and actor to display the cube edges.
vtkPolyDataMapper TubeMapper
	TubeMapper SetInputConnection [Tubes GetOutputPort]
vtkActor CubeEdges
	CubeEdges SetMapper TubeMapper
	eval [CubeEdges GetProperty] SetDiffuseColor $khaki
	[CubeEdges GetProperty] SetSpecular .4
	[CubeEdges GetProperty] SetSpecularPower 10

# Create a sphere to use as a glyph source for vtkGlyph3D.
vtkSphereSource Sphere
  Sphere SetRadius 0.04
  Sphere SetPhiResolution 20
  Sphere SetThetaResolution 20
# Remove the part of the cube with data values below 0.5.
vtkThresholdPoints ThresholdIn
  ThresholdIn SetInput Grid
  ThresholdIn ThresholdByUpper .5
# Display spheres at the vertices remaining in the cube data set after
# it was passed through vtkThresholdPoints.
vtkGlyph3D Vertices
  Vertices SetInputConnection [ThresholdIn GetOutputPort]
  Vertices SetSource [Sphere GetOutput]
# Create a mapper and actor to display the glyphs.
vtkPolyDataMapper SphereMapper
  SphereMapper SetInputConnection [Vertices GetOutputPort]
  SphereMapper ScalarVisibilityOff
vtkActor CubeVertices
  CubeVertices SetMapper SphereMapper
  eval [CubeVertices GetProperty] SetDiffuseColor $tomato
  eval [CubeVertices GetProperty] SetDiffuseColor $tomato

# Define the text for the label
vtkVectorText caseLabel
  caseLabel SetText "Case 1"

# Set up a transform to move the label to a new position.
vtkTransform aLabelTransform
  aLabelTransform Identity
  aLabelTransform Translate  -.2 0 1.25
  aLabelTransform Scale .05 .05 .05

# Move the label to a new position.
vtkTransformPolyDataFilter labelTransform
  labelTransform SetTransform aLabelTransform
  labelTransform SetInputConnection [caseLabel GetOutputPort]
  
# Create a mapper and actor to display the text.
vtkPolyDataMapper labelMapper
  labelMapper SetInputConnection [labelTransform GetOutputPort];
 
vtkActor labelActor
  labelActor SetMapper labelMapper
 
# Define the base that the cube sits on.  Create its associated mapper
# and actor.  Set the position of the actor.
vtkCubeSource baseModel
  baseModel SetXLength 1.5
  baseModel SetYLength .01
  baseModel SetZLength 1.5
vtkPolyDataMapper baseMapper
  baseMapper SetInputConnection [baseModel GetOutputPort]
vtkActor base
  base SetMapper baseMapper
  base SetPosition .5 -.09 .5

# Create the Renderer, RenderWindow, and RenderWindowInteractor
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
    renWin SetSize 640 480
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer
ren1 AddActor triangleEdgeActor
ren1 AddActor base
ren1 AddActor labelActor
ren1 AddActor CubeEdges
ren1 AddActor CubeVertices
ren1 AddActor Triangles

# Set the background color.
eval ren1 SetBackground $slate_grey

# Set the scalar values for this case of marching cubes.
case12 Scalars 0 1
# Force the grid to update.
Grid Modified

# Position the camera.
ren1 ResetCamera
[ren1 GetActiveCamera] Dolly 1.2
[ren1 GetActiveCamera] Azimuth 30
[ren1 GetActiveCamera] Elevation 20
ren1 ResetCameraClippingRange

# Render
renWin Render

# Set the user method (bound to key 'u')
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

# Withdraw the default tk window.
wm withdraw .
