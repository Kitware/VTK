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

vtkActor2D txt
  txt SetMapper mapText
  txt SetPosition 128 128
 [txt GetProperty] SetColor 1 1 0

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
