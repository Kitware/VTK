#!/usr/bin/env python

from vtkmodules.vtkCommonDataModel import (
    vtkImageData,
)

from vtkmodules.vtkInteractionImage import (
    vtkImageViewer2,
)

from vtkmodules.vtkRenderingCore import (
    vtkRenderer,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkTextProperty,
)

from vtkmodules.vtkRenderingFreeType import (
    vtkMathTextUtilities,
)

import vtkmodules.vtkRenderingMatplotlib
import vtkmodules.vtkRenderingOpenGL2
import vtkmodules.vtkRenderingUI

s = "$\\hat{H}\\psi = \\left(-\\frac{\\hbar}{2m}\\nabla^2 + V(r)\\right) \\psi = \\psi\\cdot E $"

image = vtkImageData()
utils = vtkMathTextUtilities()
utils.SetScaleToPowerOfTwo(False)
tprop = vtkTextProperty()
tprop.SetColor(1, 1, 1)
tprop.SetFontSize(50)

viewer = vtkImageViewer2()
renWin = viewer.GetRenderWindow()
utils.RenderString(s, image, tprop, renWin.GetDPI())

viewer.SetInputData(image)

iren = vtkRenderWindowInteractor()
viewer.SetupInteractor(iren)

viewer.Render()
ren = viewer.GetRenderer()
ren.ResetCamera()
ren.GetActiveCamera().Zoom(6.0)
viewer.Render()

renWin.SetMultiSamples(0)
iren = renWin.GetInteractor()
iren.Initialize()
if 'rtTester' not in locals() or rtTester.IsInteractiveModeSpecified():
    iren.Start()
