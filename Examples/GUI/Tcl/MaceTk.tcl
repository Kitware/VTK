package require vtktcl_widgets

vtkRenderer renderer

vtkRenderWindow renWin
    renWin AddRenderer renderer

set ren [vtkTkRenderWidget .ren \
        -width 300 \
        -height 300 \
        -rw renWin]
BindTkRenderWidget $ren
wm protocol . WM_DELETE_WINDOW bye

vtkSphereSource sphere
    sphere SetThetaResolution 8 
    sphere SetPhiResolution 8

vtkPolyDataMapper sphereMapper
    sphereMapper SetInput [sphere GetOutput]

vtkActor sphereActor
    sphereActor SetMapper sphereMapper

vtkConeSource cone
    cone SetResolution 6

vtkGlyph3D glyph
    glyph SetInput [sphere GetOutput]
    glyph SetSource [cone GetOutput]
    glyph SetVectorModeToUseNormal 
    glyph SetScaleModeToScaleByVector 
    glyph SetScaleFactor 0.25

vtkPolyDataMapper spikeMapper
    spikeMapper SetInput [glyph GetOutput]

vtkActor spikeActor
    spikeActor SetMapper spikeMapper

renderer AddActor sphereActor
renderer AddActor spikeActor
renderer SetBackground 1 1 1

renderer ResetCamera

set params [frame .params]

set sth [scale $params.sth \
        -from 3 -to 20 -res 1 \
        -orient horizontal \
        -label "Sphere Theta Resolution:" \
        -command setSphereThetaResolution]
$sth set [sphere GetThetaResolution]

proc setSphereThetaResolution {res} {
    sphere SetThetaResolution $res
    renWin Render
}

set sph [scale $params.sph \
        -from 3 -to 20 -res 1 \
        -orient horizontal \
        -label "Sphere Phi Resolution:" \
        -command setSpherePhiResolution]
$sph set [sphere GetPhiResolution]

proc setSpherePhiResolution {res} {
    sphere SetPhiResolution $res
    renWin Render
}

set cre [scale $params.cre \
        -from 3 -to 20 -res 1 \
        -orient horizontal \
        -label "Cone Source Resolution:" \
        -command setConeSourceResolution]
$cre set [cone GetResolution]

proc setConeSourceResolution {res} {
    cone SetResolution $res
    renWin Render
}

set gsc [scale $params.gsc \
        -from 0.1 -to 1.5 -res 0.05 \
        -orient horizontal\
        -label "Glyph Scale Factor:" \
        -command setGlyphScaleFactor]
$gsc set [glyph GetScaleFactor]

proc setGlyphScaleFactor {factor} {
    glyph SetScaleFactor $factor
    renWin Render
}

pack $sth $sph $cre $gsc -side top -anchor nw -fill both

pack $ren $params -side top -fill both -expand yes

update

proc bye {} {
    vtkCommand DeleteAllObjects
    exit
}
