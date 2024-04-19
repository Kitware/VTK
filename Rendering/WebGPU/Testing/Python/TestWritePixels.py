from vtkmodules.vtkCommonCore import (vtkUnsignedCharArray, vtkFloatArray)
from vtkmodules.vtkFiltersSources import vtkConeSource
from vtkmodules.vtkRenderingCore import (vtkActor, vtkPolyDataMapper, vtkRenderer, vtkRenderWindow, vtkRenderWindowInteractor)

import vtkmodules.vtkRenderingWebGPU # for webgpu
import vtkmodules.vtkRenderingUI
import vtkmodules.vtkInteractionStyle

from vtkmodules.util import numpy_support
import numpy as np

cone = vtkConeSource()
mapper = vtkPolyDataMapper()
actor = vtkActor()

mapper.SetInputConnection(cone.GetOutputPort())
actor.SetMapper(mapper)

renderer = vtkRenderer()
renderWindow = vtkRenderWindow()
interactor = vtkRenderWindowInteractor()

renderer.AddActor(actor)
renderer.SetBackground(100/255, 110/255, 120/255)
renderWindow.SetSize(300, 300)
renderWindow.AddRenderer(renderer)
interactor.SetRenderWindow(renderWindow)
# cone points right
renderWindow.Render()

# Get unsigned char RGBA values with cone pointing right
ucharRGBA = vtkUnsignedCharArray()
renderWindow.GetRGBACharPixelData(0, 0, 299, 299, 0, ucharRGBA)

# no cone
renderer.RemoveActor(actor)
renderWindow.Render()

# Draw the same image, but flipped along the vertical axis, cone points left.
array = numpy_support.vtk_to_numpy(ucharRGBA)
array[:, :] = np.flip(array, 0)
renderWindow.SetRGBACharPixelData(0, 0, 299, 299, ucharRGBA, 0)

# With next interaction or render, the cone should be back.
renderer.AddActor(actor)
renderWindow.Render()
# interactor.Start()
