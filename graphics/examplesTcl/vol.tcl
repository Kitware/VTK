catch {load vtktcl}
# a volume rendering example script
#
vtkVolumeRenderer volRen
vtkVolume vol
vtkStructuredPointsReader reader
vtkPolyMapper outlineMapper
vtkOutlineFilter outline

vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Read data
reader SetFileName "../../../data/ironProt.vtk"
reader Update
set range [[reader GetOutput] GetScalarRange]

# Create outline
outline SetInput [reader GetOutput]
outlineMapper SetInput [outline GetOutput]
vtkActor outline1Actor
outline1Actor SetMapper outlineMapper

ren1 SetBackground 0.1 0.2 0.4
ren1 AddActor outline1Actor
renWin SetSize 150 150
renWin Render
[ren1 GetActiveCamera] Zoom 1.5

ren1 SetVolumeRenderer volRen
volRen AddVolume vol
volRen SetStepSize 0.3
vol SetInput [reader GetOutput]
[vol GetLookupTable] SetAlphaRange 0 0.3
eval vol SetScalarRange $range

renWin Render
#renWin SetFileName "vol.tcl.ppm"
#renWin SaveImageAsPPM

wm withdraw .


