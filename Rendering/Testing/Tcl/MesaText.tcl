package require vtk
package require vtkinteraction

vtkMesaFreeTypeTextMapper textMapper
    textMapper SetInput "This is VTK"

set tprop [textMapper GetTextProperty]
    $tprop SetFontSize 25
    $tprop SetFontFamilyToArial
    $tprop SetJustificationToCentered
    $tprop BoldOn
    $tprop ItalicOn
    $tprop ShadowOn
    $tprop SetColor 0 0 1

vtkScaledTextActor textActor
    textActor SetMapper textMapper
    textActor SetPosition 0.2 0.2
    textActor SetPosition2 0.6 0.6

vtkMesaRenderer ren1

vtkXMesaRenderWindow renWin
    renWin AddRenderer ren1

vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 AddActor2D textActor

ren1 SetBackground 0.8 0.5 0.3
renWin SetSize 250 125

renWin Render

iren AddObserver UserEvent {wm deiconify .vtkInteract}

# Withdraw the tk window
wm withdraw .
