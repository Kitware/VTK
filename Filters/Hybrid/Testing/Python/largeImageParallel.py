#!/usr/bin/env python
from vtkmodules.vtkFiltersHybrid import vtkRenderLargeImage
from vtkmodules.vtkIOImport import vtk3DSImporter
from vtkmodules.vtkInteractionImage import vtkImageViewer
from vtkmodules.vtkRenderingCore import (
    vtkRenderWindow,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

ren1 = vtkRenderer()
renWin1 = vtkRenderWindow()
renWin1.AddRenderer(ren1)

importer = vtk3DSImporter()
importer.SetRenderWindow(renWin1)
importer.ComputeNormalsOn()
importer.SetFileName(VTK_DATA_ROOT + "/Data/iflamigm.3ds")
importer.Update()

importer.GetRenderer().SetBackground(0.1, 0.2, 0.4)
importer.GetRenderWindow().SetSize(150, 150)

#
# the importer created the renderer
renCollection = renWin1.GetRenderers()
renCollection.InitTraversal()

ren = renCollection.GetNextItem()

#
# change view up to +z
#
ren.GetActiveCamera().ParallelProjectionOn()
ren.GetActiveCamera().SetPosition(0, 1, 0)
ren.GetActiveCamera().SetFocalPoint(0, 0, 0)
ren.GetActiveCamera().SetViewUp(0, 0, 1)

#
# let the renderer compute good position and focal point
#
ren.ResetCamera()
ren.GetActiveCamera().Zoom(1.4)
ren1.ResetCameraClippingRange()

# render the large image
#
renderLarge = vtkRenderLargeImage()
renderLarge.SetInput(ren1)
renderLarge.SetMagnification(3)
renderLarge.Update()

viewer = vtkImageViewer()
viewer.SetInputConnection(renderLarge.GetOutputPort())
viewer.SetColorWindow(255)
viewer.SetColorLevel(127.5)
viewer.Render()

## on several opengl X window unix implementations
## multiple context deletes cause errors
## so we leak the renWin1 in this test for unix
#if renWin1.IsA('vtkXOpenGLRenderWindow'):
#    renWin1.Register(ren1)
#    dl = vtkDebugLeaks()
#    dl.SetExitError(0)
#    del dl

# iren.Initialize()
# iren.Start()
