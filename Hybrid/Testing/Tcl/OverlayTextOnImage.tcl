#
# display text over an image
#

package require vtk
package require vtkinteraction
package require vtktesting

vtkImageEllipsoidSource ellipse

vtkImageMapper mapImage
  mapImage SetInput [ellipse GetOutput]
  mapImage SetColorWindow 255
  mapImage SetColorLevel 127.5

vtkActor2D img
  img SetMapper mapImage

vtkTextMapper mapText
  mapText SetInput "Text Overlay"
  [mapText GetTextProperty] SetFontSize 15
  [mapText GetTextProperty] SetColor 0 1 1
  [mapText GetTextProperty] BoldOn
  [mapText GetTextProperty] ShadowOn

vtkActor2D txt
  txt SetMapper mapText
  txt SetPosition 138 128

vtkRenderer ren1
  ren1 AddActor2D img
  ren1 AddActor2D txt

vtkRenderWindow renWin
  renWin AddRenderer ren1

vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

iren AddObserver UserEvent {wm deiconify .vtkInteract}

renWin Render
iren Initialize

wm withdraw .
