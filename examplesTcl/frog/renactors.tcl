#
#   Render Actors Created with march.tcl
#

# get the interactor ui
source ../vtkInt.tcl
source ../colors.tcl

proc mkname {a b} {return $a$b}

# proc to make actors
# create pipeline
proc MakeActor { name r g b} {
#
    set filename  [eval mkname $name .vtk]
    set reader  [eval mkname $name PolyDataReader]
    vtkPolyDataReader $reader
       $reader SetFileName $filename
    set mapper [eval mkname $name PolyDataMapper]
    vtkPolyDataMapper $mapper
        $mapper SetInput [$reader GetOutput];
        $mapper ScalarVisibilityOff;
    set actor [ eval mkname $name Actor]
    vtkLODActor $actor
        $actor SetMapper $mapper
        eval [$actor GetProperty] SetDiffuseColor $r $g $b
        eval [$actor GetProperty] SetSpecularPower 50
        eval [$actor GetProperty] SetSpecular .5
        eval [$actor GetProperty] SetDiffuse .8
    return $actor
}

proc MakeConnectedActor { name r g b} {
#
    set filename  [eval mkname $name .vtk]
    set reader  [eval mkname $name PolyDataReader]
    vtkPolyDataReader $reader
       $reader SetFileName $filename
    set triangles [eval mkname $name TriangleFilter]
    vtkTriangleFilter $triangles
       $triangles SetInput [$reader GetOutput]
    set connect [eval mkname $name Connectivity]
    vtkConnectivityFilter $connect
       $connect SetInput [$triangles GetOutput]
       $connect SetExtractionModeToLargestRegion
    set mapper [eval mkname $name PolyDataMapper]
    vtkPolyDataMapper $mapper
        $mapper SetInput [$connect GetOutput];
        $mapper ScalarVisibilityOff;
    set actor [ eval mkname $name Actor]
    vtkLODActor $actor
        $actor SetMapper $mapper
        eval [$actor GetProperty] SetDiffuseColor $r $g $b
        eval [$actor GetProperty] SetSpecularPower 50
        eval [$actor GetProperty] SetSpecular .5
        eval [$actor GetProperty] SetDiffuse .8
    return $actor
}

# Now create the RenderWindow, Renderer and Interactor
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin


# Add the actors to the renderer, set the background and size
#

ren1 AddActor [eval MakeActor lung $powder_blue]
ren1 AddActor [eval MakeActor heart $tomato]
ren1 AddActor [eval MakeActor liver $pink]
ren1 AddActor [eval MakeActor duodenum $orange]
ren1 AddActor [eval MakeActor blood $salmon]
ren1 AddActor [eval MakeActor brainbin $beige]
ren1 AddActor [eval MakeActor eye_retna $misty_rose]
ren1 AddActor [eval MakeActor eye_white $white]
ren1 AddActor [eval MakeActor ileum $raspberry]
ren1 AddActor [eval MakeActor kidney $banana]
ren1 AddActor [eval MakeActor l_intestine $peru]
ren1 AddActor [eval MakeActor nerve $carrot]
ren1 AddActor [eval MakeActor spleen $violet]
ren1 AddActor [eval MakeActor stomach $plum]
ren1 AddActor [eval MakeActor skeleton $wheat]
ren1 AddActor [eval MakeActor skin $lime_green]
[skinActor GetProperty] SetOpacity .4

ren1 SetBackground 0.2 0.3 0.4;
renWin SetSize 450 450;
[ren1 GetActiveCamera] SetViewUp 0 -1 0;
[ren1 GetActiveCamera] Azimuth 230
[ren1 GetActiveCamera] Elevation 30
[ren1 GetActiveCamera] Dolly 1.75
iren Initialize;

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract};

# prevent the tk window from showing up then start the event loop
wm withdraw .
