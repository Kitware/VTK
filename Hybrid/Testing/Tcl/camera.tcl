package require vtk
package require vtkinteraction

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create a camera model
vtkConeSource camCS
camCS SetHeight 1.5
camCS SetResolution 12
camCS SetRadius 0.4

vtkCubeSource camCBS
camCBS SetXLength 1.5
camCBS SetZLength 0.8
camCBS SetCenter 0.4 0 0

vtkAppendFilter camAPD
camAPD AddInput [camCS GetOutput]
camAPD AddInput [camCBS GetOutput]

vtkDataSetMapper camMapper
    camMapper SetInput [camAPD GetOutput]
vtkLODActor camActor
    camActor SetMapper camMapper
camActor SetScale 2 2 2

# draw the arrows
vtkPolyData pd
vtkCellArray ca
vtkPoints fp
fp InsertNextPoint 0 1 0
fp InsertNextPoint 8 1 0
fp InsertNextPoint 8 2 0
fp InsertNextPoint 10 0.01 0
fp InsertNextPoint 8 -2 0
fp InsertNextPoint 8 -1 0
fp InsertNextPoint 0 -1 0
ca InsertNextCell 7
ca InsertCellPoint 0
ca InsertCellPoint 1
ca InsertCellPoint 2
ca InsertCellPoint 3
ca InsertCellPoint 4
ca InsertCellPoint 5
ca InsertCellPoint 6
pd SetPoints fp
pd SetPolys ca

vtkPolyData pd2
vtkCellArray ca2
vtkPoints fp2
fp2 InsertNextPoint 0 1 0
fp2 InsertNextPoint 8 1 0
fp2 InsertNextPoint 8 2 0
fp2 InsertNextPoint 10 0.01 0;#prevents degenerate triangles
ca2 InsertNextCell 4
ca2 InsertCellPoint 0
ca2 InsertCellPoint 1
ca2 InsertCellPoint 2
ca2 InsertCellPoint 3
pd2 SetPoints fp2
pd2 SetLines ca2

vtkImplicitModeller arrowIM
arrowIM SetInput pd
arrowIM SetSampleDimensions 50 20 8

vtkContourFilter arrowCF
arrowCF SetInput [arrowIM GetOutput]
arrowCF SetValue 0 0.2

vtkWarpTo arrowWT
arrowWT SetInput [arrowCF GetOutput]
arrowWT SetPosition 5 0 5
arrowWT SetScaleFactor 0.85
arrowWT AbsoluteOn

vtkTransform arrowT
arrowT RotateY 60
arrowT Translate -1.33198 0 -1.479
arrowT Scale 1 0.5 1

vtkTransformFilter arrowTF
arrowTF SetInput [arrowWT GetOutput]
arrowTF SetTransform arrowT

vtkDataSetMapper arrowMapper
arrowMapper SetInput [arrowTF GetOutput]
arrowMapper ScalarVisibilityOff

# draw the azimuth arrows
vtkLODActor a1Actor
a1Actor SetMapper arrowMapper
a1Actor RotateZ 180
a1Actor SetPosition 1 0 -1
[a1Actor GetProperty] SetColor 1 0.3 0.3
[a1Actor GetProperty] SetSpecularColor 1 1 1
[a1Actor GetProperty] SetSpecular 0.3
[a1Actor GetProperty] SetSpecularPower 20
[a1Actor GetProperty] SetAmbient 0.2
[a1Actor GetProperty] SetDiffuse 0.8

vtkLODActor a2Actor
a2Actor SetMapper arrowMapper
a2Actor RotateZ 180
a2Actor RotateX 180
a2Actor SetPosition 1 0 1
[a2Actor GetProperty] SetColor 1 0.3 0.3
[a2Actor GetProperty] SetSpecularColor 1 1 1
[a2Actor GetProperty] SetSpecular 0.3
[a2Actor GetProperty] SetSpecularPower 20
[a2Actor GetProperty] SetAmbient 0.2
[a2Actor GetProperty] SetDiffuse 0.8

# draw the elevation arrows
vtkLODActor a3Actor
a3Actor SetMapper arrowMapper
a3Actor RotateZ 180
a3Actor RotateX 90
a3Actor SetPosition 1 -1 0
[a3Actor GetProperty] SetColor 0.3 1 0.3
[a3Actor GetProperty] SetSpecularColor 1 1 1
[a3Actor GetProperty] SetSpecular 0.3
[a3Actor GetProperty] SetSpecularPower 20
[a3Actor GetProperty] SetAmbient 0.2
[a3Actor GetProperty] SetDiffuse 0.8

vtkLODActor a4Actor
a4Actor SetMapper arrowMapper
a4Actor RotateZ 180
a4Actor RotateX -90
a4Actor SetPosition 1 1 0
[a4Actor GetProperty] SetColor 0.3 1 0.3
[a4Actor GetProperty] SetSpecularColor 1 1 1
[a4Actor GetProperty] SetSpecular 0.3
[a4Actor GetProperty] SetSpecularPower 20
[a4Actor GetProperty] SetAmbient 0.2
[a4Actor GetProperty] SetDiffuse 0.8

# draw the DOP
vtkTransform arrowT2
arrowT2 Scale 1 0.6 1
arrowT2 RotateY 90

vtkTransformPolyDataFilter arrowTF2
arrowTF2 SetInput pd2
arrowTF2 SetTransform arrowT2

vtkRotationalExtrusionFilter arrowREF
arrowREF SetInput [arrowTF2 GetOutput]
arrowREF CappingOff
arrowREF SetResolution 30

vtkPolyDataMapper spikeMapper
spikeMapper SetInput [arrowREF GetOutput]

vtkLODActor a5Actor
a5Actor SetMapper spikeMapper
a5Actor SetScale .3 .3 .6
a5Actor RotateY 90
a5Actor SetPosition -2 0 0
[a5Actor GetProperty] SetColor 1 0.3 1
[a5Actor GetProperty] SetAmbient 0.2
[a5Actor GetProperty] SetDiffuse 0.8

# focal point
vtkSphereSource fps
fps SetRadius 0.5
vtkPolyDataMapper fpMapper
fpMapper SetInput [fps GetOutput]
vtkLODActor fpActor
fpActor SetMapper fpMapper
fpActor SetPosition -9 0 0
[fpActor GetProperty] SetSpecularColor 1 1 1
[fpActor GetProperty] SetSpecular 0.3
[fpActor GetProperty] SetAmbient 0.2
[fpActor GetProperty] SetDiffuse 0.8
[fpActor GetProperty] SetSpecularPower 20

# create the roll arrows
vtkWarpTo arrowWT2
arrowWT2 SetInput [arrowCF GetOutput]
arrowWT2 SetPosition 5 0 2.5
arrowWT2 SetScaleFactor 0.95
arrowWT2 AbsoluteOn

vtkTransform arrowT3
arrowT3 Translate -2.50358 0 -1.70408
arrowT3 Scale 0.5 0.3 1

vtkTransformFilter arrowTF3
arrowTF3 SetInput [arrowWT2 GetOutput]
arrowTF3 SetTransform arrowT3

vtkDataSetMapper arrowMapper2
arrowMapper2 SetInput [arrowTF3 GetOutput]
arrowMapper2 ScalarVisibilityOff

# draw the roll arrows
vtkLODActor a6Actor
a6Actor SetMapper arrowMapper2
a6Actor RotateZ 90
a6Actor SetPosition -4 0 0
a6Actor SetScale 1.5 1.5 1.5
[a6Actor GetProperty] SetColor 1 1 0.3
[a6Actor GetProperty] SetSpecularColor 1 1 1
[a6Actor GetProperty] SetSpecular 0.3
[a6Actor GetProperty] SetSpecularPower 20
[a6Actor GetProperty] SetAmbient 0.2
[a6Actor GetProperty] SetDiffuse 0.8

# Add the actors to the renderer, set the background and size
ren1 AddActor camActor
ren1 AddActor a1Actor
ren1 AddActor a2Actor
ren1 AddActor a3Actor
ren1 AddActor a4Actor
ren1 AddActor a5Actor
ren1 AddActor a6Actor
ren1 AddActor fpActor
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 300 300

# render the image
iren AddObserver UserEvent {wm deiconify .vtkInteract}
set cam1 [ren1 GetActiveCamera]
$cam1 Zoom 1.5
$cam1 Azimuth 150
$cam1 Elevation 30

iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .

# for testing
set threshold 15


