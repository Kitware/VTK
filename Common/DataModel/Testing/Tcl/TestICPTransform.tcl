package require vtk
package require vtkinteraction

vtkRenderWindow renWin

vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Create objects

array set sscale {
    s2 {0.7 0.7 0.7}
    s3 {0.5 0.5 0.5}
}
array set scenter {
    s2 {-0.25  0.25  0.0}
    s3 { 0.4  -0.3  0.0}
}
array set scolors {
    s2 {0.2 0.6 0.1}
    s3 {0.1 0.2 0.6}
}

for {set sidx 1} {$sidx <= 3} {incr sidx} {
    vtkSuperquadricSource s${sidx}
        s${sidx} ToroidalOff
        s${sidx} SetThetaResolution 20
        s${sidx} SetPhiResolution 20
        s${sidx} SetPhiRoundness [expr 0.7 + ($sidx-2)*0.4]
        s${sidx} SetThetaRoundness [expr 0.85 + ($sidx-1)*0.4]
        if [info exists sscale(s${sidx})] {
            eval s${sidx} SetScale $sscale(s${sidx})
        }
        if [info exists scenter(s${sidx})] {
            eval s${sidx} SetCenter $scenter(s${sidx})
        }
        s${sidx} Update
}

# Create renderers

for {set ridx 1} {$ridx <= 3} {incr ridx} {

    vtkRenderer ren${ridx}
        ren${ridx} SetViewport [expr ($ridx-1)/3.0] 0.0 [expr ($ridx)/3.0] 1.0
        ren${ridx} SetBackground 0.7 0.8 1.0
    set cam [ren${ridx} GetActiveCamera]
        $cam SetPosition 1.7 1.4 1.7
    renWin AddRenderer ren${ridx}

    # renderer 1 has all 3 objects, render i has object 1 and i (i=2, 3)
    # add actors (corresponding to the objects) to each renderer
    # and ICP transforms from objects i or to 1.
    # object 1 has feature edges too.

    for {set sidx 1} {$sidx <= 3} {incr sidx} {

        if {$ridx == 1 || $sidx == 1 || $ridx == $sidx} {
            vtkPolyDataMapper ren${ridx}s${sidx}m
                ren${ridx}s${sidx}m SetInputConnection [s${sidx} GetOutputPort]

            vtkActor ren${ridx}s${sidx}a
                ren${ridx}s${sidx}a SetMapper ren${ridx}s${sidx}m

            set prop [ren${ridx}s${sidx}a GetProperty]
            if [info exists scolors(s${sidx})] {
                eval $prop SetColor $scolors(s${sidx})
            }

            if {$sidx == 1} {
                $prop SetOpacity 0.2

                vtkFeatureEdges ren${ridx}s${sidx}fe
                    ren${ridx}s${sidx}fe SetInputConnection [s${sidx} GetOutputPort]
                    ren${ridx}s${sidx}fe BoundaryEdgesOn
                    ren${ridx}s${sidx}fe ColoringOff
                    ren${ridx}s${sidx}fe ManifoldEdgesOff

                vtkPolyDataMapper ren${ridx}s${sidx}fem
                    ren${ridx}s${sidx}fem SetInputConnection \
                                          [ren${ridx}s${sidx}fe GetOutputPort]
                    ren${ridx}s${sidx}fem \
                                    SetResolveCoincidentTopologyToPolygonOffset

                vtkActor ren${ridx}s${sidx}fea
                    ren${ridx}s${sidx}fea SetMapper ren${ridx}s${sidx}fem

                ren${ridx} AddActor ren${ridx}s${sidx}fea
            }

            ren${ridx} AddActor ren${ridx}s${sidx}a
        }

        if {$ridx > 1 && $ridx == $sidx} {
            vtkIterativeClosestPointTransform ren${ridx}icp${sidx}
                ren${ridx}icp${sidx} SetSource [s${sidx} GetOutput]
                ren${ridx}icp${sidx} SetTarget [s1 GetOutput]
                ren${ridx}icp${sidx} SetCheckMeanDistance 1
                ren${ridx}icp${sidx} SetMaximumMeanDistance 0.001
                ren${ridx}icp${sidx} SetMaximumNumberOfIterations 30
                ren${ridx}icp${sidx} SetMaximumNumberOfLandmarks 50
            ren${ridx}s${sidx}a SetUserTransform ren${ridx}icp${sidx}
        }
    }
}

ren3icp3 StartByMatchingCentroidsOn

renWin SetSize 400 100
renWin Render

catch {
    iren AddObserver UserEvent {wm deiconify .vtkInteract}
}

wm withdraw .

