package require vtk

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkBYUReader byuReader
    byuReader SetGeometryFileName $VTK_DATA_ROOT/Data/teapot.g

vtkPolyDataMapper   byuMapper
    byuMapper SetInput [byuReader GetOutput]

for { set i 0 } { $i < 9 } { incr i } {
    vtkActor byuActor${i}
    byuActor${i} SetMapper byuMapper
    ren1 AddActor byuActor${i}

    vtkHull hull${i}
    hull${i} SetInput [byuReader GetOutput]

    vtkPolyDataMapper hullMapper${i}
    hullMapper${i} SetInput [hull${i} GetOutput]
    
    vtkActor hullActor${i}
    hullActor${i} SetMapper hullMapper${i}
    [hullActor${i} GetProperty] SetColor 1 0 0
    [hullActor${i} GetProperty] SetAmbient 0.2
    [hullActor${i} GetProperty] SetDiffuse 0.8
    [hullActor${i} GetProperty] SetRepresentationToWireframe

    ren1 AddActor hullActor${i}
}

byuReader Update

set diagonal [byuActor0 GetLength]
set i 0
for { set j -1 } { $j < 2 } { incr j } {
    for { set k -1 } { $k < 2 } { incr k } {
	byuActor${i} AddPosition [expr $k * $diagonal] [expr $j * $diagonal] 0
	hullActor${i} AddPosition [expr $k * $diagonal] [expr $j * $diagonal] 0
	incr i
    }
}

hull0 AddCubeFacePlanes
hull1 AddCubeEdgePlanes
hull2 AddCubeVertexPlanes
hull3 AddCubeFacePlanes
hull3 AddCubeEdgePlanes
hull3 AddCubeVertexPlanes
hull4 AddRecursiveSpherePlanes 0
hull5 AddRecursiveSpherePlanes 1
hull6 AddRecursiveSpherePlanes 2
hull7 AddRecursiveSpherePlanes 3
hull8 AddRecursiveSpherePlanes 4

# Add the actors to the renderer, set the background and size
#
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 500 500

iren Initialize
renWin Render
[ren1 GetActiveCamera] Zoom 1.5
renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .

# for testing
set threshold 15
