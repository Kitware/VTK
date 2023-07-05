#!/usr/bin/env python
import sys
from vtkmodules.vtkCommonCore import (
    VTK_LINEAR_INTERPOLATION,
    vtkObjectFactory,
)
from vtkmodules.vtkCommonDataModel import vtkPiecewiseFunction
from vtkmodules.vtkIOImage import vtkNrrdReader
from vtkmodules.vtkRenderingCore import (
    vtkColorTransferFunction,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
    vtkShaderProperty,
    vtkVolume,
    vtkVolumeProperty,
)
from vtkmodules.vtkRenderingVolume import vtkGPUVolumeRayCastMapper
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
import vtkmodules.vtkRenderingVolumeOpenGL2
import vtkmodules.test.Testing
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

'''
  Prevent .pyc files from being created.
  Stops the vtk source being polluted
  by .pyc files.
'''
sys.dont_write_bytecode = True

# Disable object factory override for vtkNrrdReader
vtkObjectFactory.SetAllEnableFlags(False, "vtkNrrdReader", "vtkPNrrdReader")

reader = vtkNrrdReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/tooth.nhdr")
reader.Update()

volumeProperty = vtkVolumeProperty()
volumeProperty.ShadeOn()
volumeProperty.SetInterpolationType(VTK_LINEAR_INTERPOLATION)

range = reader.GetOutput().GetScalarRange()

# Prepare 1D Transfer Functions
ctf = vtkColorTransferFunction()
ctf.AddRGBPoint(0, 0.0, 0.0, 0.0)
ctf.AddRGBPoint(510, 0.4, 0.4, 1.0)
ctf.AddRGBPoint(640, 1.0, 1.0, 1.0)
ctf.AddRGBPoint(range[1], 0.9, 0.1, 0.1)

pf = vtkPiecewiseFunction()
pf.AddPoint(0, 0.00)
pf.AddPoint(510, 0.00)
pf.AddPoint(640, 0.5)
pf.AddPoint(range[1], 0.4)

volumeProperty.SetScalarOpacity(pf)
volumeProperty.SetColor(ctf)
volumeProperty.SetShade(1)

mapper = vtkGPUVolumeRayCastMapper()
mapper.SetInputConnection(reader.GetOutputPort())
mapper.SetUseJittering(1)

# Modify the shader to color based on the depth of the translucent voxel
shaderProperty = vtkShaderProperty()
shaderProperty.AddFragmentShaderReplacement(
    "//VTK::Base::Dec",      # Source string to replace
    True,                    # before the standard replacements
    "//VTK::Base::Dec"       # We still want the default
    "\n bool l_updateDepth;"
    "\n vec3 l_opaqueFragPos;",
    False                    # only do it once i.e. only replace the first match
)
shaderProperty.AddFragmentShaderReplacement(
    "//VTK::Base::Init",
    True,
    "//VTK::Base::Init\n"
    "\n l_updateDepth = true;"
    "\n l_opaqueFragPos = vec3(0.0);",
    False
)
shaderProperty.AddFragmentShaderReplacement(
    "//VTK::Base::Impl",
    True,
    "//VTK::Base::Impl"
    "\n    if(!g_skip && g_srcColor.a > 0.0 && l_updateDepth)"
    "\n      {"
    "\n      l_opaqueFragPos = g_dataPos;"
    "\n      l_updateDepth = false;"
    "\n      }",
    False
)
shaderProperty.AddFragmentShaderReplacement(
    "//VTK::RenderToImage::Exit",
    True,
    "//VTK::RenderToImage::Exit"
    "\n  if (l_opaqueFragPos == vec3(0.0))"
    "\n    {"
    "\n    fragOutput0 = vec4(0.0);"
    "\n    }"
    "\n  else"
    "\n    {"
    "\n    vec4 depthValue = in_projectionMatrix * in_modelViewMatrix *"
    "\n                      in_volumeMatrix[0] * in_textureDatasetMatrix[0] *"
    "\n                      vec4(l_opaqueFragPos, 1.0);"
    "\n    depthValue /= depthValue.w;"
    "\n    fragOutput0 = vec4(vec3(0.5 * (gl_DepthRange.far -"
    "\n                       gl_DepthRange.near) * depthValue.z + 0.5 *"
    "\n                      (gl_DepthRange.far + gl_DepthRange.near)), 1.0);"
    "\n    }",
    False
)

volume = vtkVolume()
volume.SetShaderProperty(shaderProperty)
volume.SetMapper(mapper)
volume.SetProperty(volumeProperty)

renWin = vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.SetSize(300, 300)

ren = vtkRenderer()
renWin.AddRenderer(ren)

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren.AddVolume(volume)
ren.GetActiveCamera().Elevation(-60.0)
ren.ResetCamera()
ren.GetActiveCamera().Zoom(1.3)

renWin.Render()
iren.Start()
