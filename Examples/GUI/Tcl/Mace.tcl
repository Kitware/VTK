package require vtktcl

vtkRenderer renderer

vtkRenderWindow renWin
    renWin AddRenderer renderer

vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

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
renWin SetSize 300 300

# interact with data
renWin Render 

wm withdraw .
