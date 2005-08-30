package require vtk
package require vtkinteraction

# Created oriented text

vtkTextSource text0Source
    text0Source SetText "Text Source with Scalars (default)"

vtkPolyDataMapper text0Mapper
    text0Mapper SetInputConnection [text0Source GetOutputPort]

vtkActor text0Actor
    text0Actor SetMapper text0Mapper
    text0Actor SetScale .1 .1 .1
    text0Actor AddPosition 0 2 0

vtkTextSource text1Source
    text1Source SetText "Text Source with Scalars"
    text1Source SetForegroundColor 1 0 0
    text1Source SetBackgroundColor 1 1 1

vtkPolyDataMapper text1Mapper
    text1Mapper SetInputConnection [text1Source GetOutputPort]

vtkActor text1Actor
    text1Actor SetMapper text1Mapper
    text1Actor SetScale .1 .1 .1

vtkTextSource text2Source
    text2Source SetText "Text Source without Scalars"
    text2Source BackingOff

vtkPolyDataMapper text2Mapper
    text2Mapper SetInputConnection [text2Source GetOutputPort]
    text2Mapper ScalarVisibilityOff

vtkActor text2Actor
    text2Actor SetMapper text2Mapper
    [text2Actor GetProperty] SetColor 1 1 0
    text2Actor SetScale .1 .1 .1
    text2Actor AddPosition 0 -2 0

vtkVectorText text3Source
    text3Source SetText "Vector Text"

vtkPolyDataMapper text3Mapper
    text3Mapper SetInputConnection [text3Source GetOutputPort]
    text3Mapper ScalarVisibilityOff

vtkActor text3Actor
    text3Actor SetMapper text3Mapper
    [text3Actor GetProperty] SetColor .1 1 0
    text3Actor AddPosition 0 -4 0

# create graphics stuff
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
    renWin SetSize 350 100

vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 AddActor text0Actor
ren1 AddActor text1Actor
ren1 AddActor text2Actor
ren1 AddActor text3Actor
ren1 ResetCamera
[ren1 GetActiveCamera] Zoom 3
ren1 SetBackground .1 .2 .4

renWin Render

iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

wm withdraw .
