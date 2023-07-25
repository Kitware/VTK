#!/usr/bin/env python
import sys
from vtkmodules.vtkCommonCore import (
    VTK_OBJECT,
    vtkCommand,
)
from vtkmodules.vtkFiltersCore import vtkTriangleMeshPointNormals
from vtkmodules.vtkIOPLY import vtkPLYReader
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
from vtkmodules.vtkRenderingOpenGL2 import vtkOpenGLPolyDataMapper
from vtkmodules.util.misc import calldata_type
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot

'''
  Prevent .pyc files from being created.
  Stops the vtk source being polluted
  by .pyc files.
'''
sys.dont_write_bytecode = True


@calldata_type(VTK_OBJECT)
def vtkShaderCallback(caller, event, calldata):
    program = calldata
    if program is not None:
        diffuseColor = [0.4, 0.7, 0.6]
        program.SetUniform3f("diffuseColorUniform", diffuseColor)


renWin = vtkRenderWindow()
renWin.SetSize(400, 400)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
ren = vtkRenderer()
ren.SetBackground(0.0, 0.0, 0.0)
ren.GradientBackgroundOn()
renWin.AddRenderer(ren)
actor = vtkActor()
ren.AddActor(actor)
reader = vtkPLYReader()
reader.SetFileName(vtkGetDataRoot() + "/Data/dragon.ply")
norms = vtkTriangleMeshPointNormals()
norms.SetInputConnection(reader.GetOutputPort())
mapper = vtkOpenGLPolyDataMapper()
mapper.SetInputConnection(norms.GetOutputPort())
actor.SetMapper(mapper)
actor.GetProperty().SetAmbientColor(0.2, 0.2, 1.0)
actor.GetProperty().SetDiffuseColor(1.0, 0.65, 0.7)
actor.GetProperty().SetSpecularColor(1.0, 1.0, 1.0)
actor.GetProperty().SetSpecular(0.5)
actor.GetProperty().SetDiffuse(0.7)
actor.GetProperty().SetAmbient(0.5)
actor.GetProperty().SetSpecularPower(20.0)
actor.GetProperty().SetOpacity(1.0)

sp = actor.GetShaderProperty()
sp.SetVertexShaderCode(
    "//VTK::System::Dec\n"
    "in vec4 vertexMC;\n"
    "//VTK::Normal::Dec\n"
    "uniform mat4 MCDCMatrix;\n"
    "void main () {\n"
    "  normalVCVSOutput = normalMatrix * normalMC;\n"
    "  vec4 tmpPos = MCDCMatrix * vertexMC;\n"
    "  gl_Position = tmpPos*vec4(0.2+0.8*abs(tmpPos.x),0.2+0.8*abs(tmpPos.y),1.0,1.0);\n"
    "}\n"
)
sp.SetFragmentShaderCode(
    "//VTK::System::Dec\n"
    "//VTK::Output::Dec\n"
    "in vec3 normalVCVSOutput;\n"
    "uniform vec3 diffuseColorUniform;\n"
    "void main () {\n"
    "  float df = max(0.0, normalVCVSOutput.z);\n"
    "  float sf = pow(df, 20.0);\n"
    "  vec3 diffuse = df * diffuseColorUniform;\n"
    "  vec3 specular = sf * vec3(0.4,0.4,0.4);\n"
    "  gl_FragData[0] = vec4(0.3*abs(normalVCVSOutput) + 0.7*diffuse + specular, 1.0);\n"
    "}\n"
)

mapper.AddObserver(vtkCommand.UpdateShaderEvent, vtkShaderCallback)

renWin.Render()
ren.GetActiveCamera().SetPosition(-0.2, 0.4, 1)
ren.GetActiveCamera().SetFocalPoint(0, 0, 0)
ren.GetActiveCamera().SetViewUp(0, 1, 0)
ren.ResetCamera()
ren.GetActiveCamera().Zoom(2.0)
renWin.Render()
iren.Start()
