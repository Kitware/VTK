#!/usr/bin/env python
from vtkmodules.vtkIOImport import vtkVRMLImporter
from vtkmodules.vtkRenderingCore import (
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
importer = vtkVRMLImporter()
importer.SetRenderWindow(renWin)
importer.SetFileName(VTK_DATA_ROOT + "/Data/bot2.wrl")
importer.Read()
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
importer.GetRenderer().SetBackground(0.1,0.2,0.4)
importer.GetRenderWindow().SetSize(300,300)
#
# the importer created the renderer
renCollection = renWin.GetRenderers()
renCollection.InitTraversal()
ren = renCollection.GetNextItem()
#
# change view up to +z
#
ren.GetActiveCamera().SetPosition(-3.25303,3.46205,3.15906)
ren.GetActiveCamera().SetFocalPoint(0,0,0)
ren.GetActiveCamera().SetViewUp(0.564063,0.825024,-0.0341876)
#
# let the renderer compute good position and focal point
#
ren.ResetCamera()
ren.GetActiveCamera().Dolly(1.75)
ren1.ResetCameraClippingRange()
# render the image
#
iren.Initialize()
# --- end of script --
