catch {load vtktcl}
# get the interactor ui
source ../../examplesTcl/vtkInt.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin


proc create_profile {major minor {resolution 20}} {
    set pi 3.141592
    set twopi [expr 2 * $pi]
    vtkFloatPoints points
    vtkCellArray poly
    set theta 0.0
    set delta [expr $twopi/$resolution]
    poly InsertNextCell $resolution
    set i 0
    while {$theta < $twopi} {
	set x [expr cos($theta) * $minor + $major]
	set z [expr sin($theta) * $minor]
	points InsertPoint $i $x 0.0 $z
	poly InsertCellPoint $i
	set i [expr $i + 1]
	set theta [expr $theta + $delta]
    }
}

# define torus with major radius of 1.25 and minor radius of 0.25
create_profile 1.25 0.25

vtkPolyData profile
    profile SetPoints points
    profile SetPolys poly

# extrude profile to make spring
#
vtkRotationalExtrusionFilter extrude
    extrude SetInput profile
    extrude SetResolution 100
    extrude SetTranslation 0
    extrude SetDeltaRadius 0
    extrude SetAngle 360.0;
    
vtkPolyDataNormals normals
    normals SetInput [extrude GetOutput]
    normals SetFeatureAngle 60

vtkPolyDataMapper map
    map SetInput [normals GetOutput]

vtkActor torus
    torus SetMapper map
    [torus GetProperty] SetColor 0.6902 0.7686 0.8706
    [torus GetProperty] SetDiffuse 0.7
    [torus GetProperty] SetSpecular 0.4
    [torus GetProperty] SetSpecularPower 20
    [torus GetProperty] BackfaceCullingOn

# Add the actors to the renderer, set the background and size
#
ren1 AddActor torus
ren1 SetBackground 1 1 1
renWin SetSize 500 500

set cam1 [ren1 GetActiveCamera]
$cam1 Azimuth 90

renWin Render
#renWin SetFileName "torus.tcl.ppm"
#renWin SaveImageAsPPM

iren SetUserMethod {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .



