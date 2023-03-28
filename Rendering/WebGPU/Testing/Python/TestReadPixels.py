from vtkmodules.vtkCommonCore import vtkUnsignedCharArray
from vtkmodules.vtkFiltersSources import vtkConeSource
from vtkmodules.vtkRenderingCore import (vtkActor, vtkPolyDataMapper, vtkRenderer, vtkRenderWindow, vtkRenderWindowInteractor)

import vtkmodules.vtkRenderingWebGPU # for webgpu
import vtkmodules.vtkRenderingUI
import vtkmodules.vtkInteractionStyle

cone = vtkConeSource()
mapper = vtkPolyDataMapper()
actor = vtkActor()

mapper.SetInputConnection(cone.GetOutputPort())
actor.SetMapper(mapper)

renderer = vtkRenderer()
renderWindow = vtkRenderWindow()
interactor = vtkRenderWindowInteractor()

renderer.AddActor(actor)
renderWindow.SetSize(300, 300)
renderWindow.AddRenderer(renderer)
interactor.SetRenderWindow(renderWindow)
renderWindow.Render()

arr = vtkUnsignedCharArray()
breakpoint()

# renderWindow.GetPixelData(0, 0, 0, 0, 0, arr, 0)

interactor.Start()
