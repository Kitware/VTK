from vtkmodules.vtkCommonCore import (vtkUnsignedCharArray, vtkFloatArray)
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
renderer.SetBackground(100/255, 110/255, 120/255)
renderWindow.SetSize(300, 300)
renderWindow.AddRenderer(renderer)
interactor.SetRenderWindow(renderWindow)
renderWindow.Render()

# Verify background color as unsigned char RGBA values
ucharRGBA = vtkUnsignedCharArray()
renderWindow.GetRGBACharPixelData(0, 0, 299, 299, 0, ucharRGBA)
assert(ucharRGBA.GetTuple(0) == (100.0, 110.0, 120.0, 255.0))
assert(ucharRGBA.GetTuple(299 * 299) == (100.0, 110.0, 120.0, 255.0))

# Verify background color as normalized float32 RGBA values
f32RGBA = vtkFloatArray()
renderWindow.GetRGBAPixelData(0, 0, 299, 299, 0, f32RGBA)
assert(int(f32RGBA.GetTuple(0)[0] * 255) == 100)
assert(int(f32RGBA.GetTuple(0)[1] * 255) == 110)
assert(int(f32RGBA.GetTuple(0)[2] * 255) == 120)
assert(int(f32RGBA.GetTuple(0)[3] * 255) == 255)
assert(int(f32RGBA.GetTuple(299)[0] * 255) == 100)
assert(int(f32RGBA.GetTuple(299)[1] * 255) == 110)
assert(int(f32RGBA.GetTuple(299)[2] * 255) == 120)
assert(int(f32RGBA.GetTuple(299)[3] * 255) == 255)

# Verify background color as unsigned char RGB values
ucharRGB = vtkUnsignedCharArray()
renderWindow.GetPixelData(0, 0, 299, 299, 0, ucharRGB, 0)
assert(ucharRGB.GetTuple(0) == (100.0, 110.0, 120.0))
assert(ucharRGB.GetTuple(299 * 299) == (100.0, 110.0, 120.0))

# Verify background color as normalized float32 RGB values
f32RGB = vtkFloatArray()
renderWindow.GetRGBAPixelData(0, 0, 299, 299, 0, f32RGB, 0)
assert(int(f32RGB.GetTuple(0)[0] * 255) == 100)
assert(int(f32RGB.GetTuple(0)[1] * 255) == 110)
assert(int(f32RGB.GetTuple(0)[2] * 255) == 120)
assert(int(f32RGB.GetTuple(299)[0] * 255) == 100)
assert(int(f32RGB.GetTuple(299)[1] * 255) == 110)
assert(int(f32RGB.GetTuple(299)[2] * 255) == 120)
