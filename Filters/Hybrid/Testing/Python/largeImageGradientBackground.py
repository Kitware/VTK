#!/usr/bin/env python

# Test vtkRenderLargeImage with a renderer that uses a gradient background

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
renWin1.SetMultiSamples(0)

importer = vtk3DSImporter()
importer.SetRenderWindow(renWin1)
importer.ComputeNormalsOn()
importer.SetFileName(VTK_DATA_ROOT + "/Data/iflamigm.3ds")
importer.Update()

importer.GetRenderer().SetBackground(0.7568627450980392, 0.7647058823529412, 0.9098039215686275)
importer.GetRenderer().SetBackground2(0.4549019607843137, 0.4705882352941176, 0.7450980392156863)
importer.GetRenderer().SetGradientBackground(True)
importer.GetRenderWindow().SetSize(150, 150)

# the importer created the renderer
renCollection = renWin1.GetRenderers()
renCollection.InitTraversal()

ren = renCollection.GetNextItem()

ren.GetActiveCamera().SetPosition(0, 1, 0)
ren.GetActiveCamera().SetFocalPoint(0, 0, 0)
ren.GetActiveCamera().SetViewUp(0, 0, 1)

ren.ResetCamera()
ren.GetActiveCamera().Dolly(1.4)
ren1.ResetCameraClippingRange()

# render the large image
renderLarge = vtkRenderLargeImage()
renderLarge.SetInput(ren1)
renderLarge.SetMagnification(3)
renderLarge.Update()

viewer = vtkImageViewer()
viewer.SetInputConnection(renderLarge.GetOutputPort())
viewer.SetColorWindow(255)
viewer.SetColorLevel(127.5)
viewer.Render()
