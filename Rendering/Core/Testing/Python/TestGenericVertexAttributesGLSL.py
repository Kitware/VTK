#!/usr/bin/env python

# tests the support to pass generic vertex attributes to be used in Cg shaders.
xmlMaterial = '''<?xml version="1.0" encoding="UTF-8"?>
<Material name="GenericAttributes1">
  <Shader
    scope="Vertex"
    name="VertexShader"
    location="Inline"
    language="GLSL"
    entry="main">
    attribute vec3 genAttrVector;
    varying vec4 color;

    void main(void)
    {
      gl_Position = gl_ModelViewProjectionMatrix *gl_Vertex;
      color = vec4(normalize(genAttrVector), 1.0);
    }
  </Shader>

  <Shader scope="Fragment" name="FragmentShader" location="Inline"
    language="GLSL" entry="main">
    varying vec4 color;
    void main(void)
    {
      gl_FragColor = color;
    }
  </Shader>
</Material>'''

renWin = vtk.vtkRenderWindow()
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
renderer = vtk.vtkRenderer()
renWin.AddRenderer(renderer)
src1 = vtk.vtkSphereSource()
src1.SetRadius(5)
src1.SetPhiResolution(20)
src1.SetThetaResolution(20)
randomVectors = vtk.vtkBrownianPoints()
randomVectors.SetMinimumSpeed(0)
randomVectors.SetMaximumSpeed(1)
randomVectors.SetInputConnection(src1.GetOutputPort())
mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(randomVectors.GetOutputPort())
actor = vtk.vtkActor()
actor.SetMapper(mapper)
# Load the material. Here, we are loading a material
# defined in the Vtk Library. One can also specify
# a filename to a material description xml.
actor.GetProperty().LoadMaterialFromString(xmlMaterial)
# Set red color to show if shading fails.
actor.GetProperty().SetColor(1.0,0,0)
# Turn shading on. Otherwise, shaders are not used.
actor.GetProperty().ShadingOn()
# Map PointData.BrownianVectors (all 3 components) to genAttrVector
mapper.MapDataArrayToVertexAttribute("genAttrVector","BrownianVectors",0,-1)
renderer.AddActor(actor)
renderer.SetBackground(0.5,0.5,0.5)
renWin.Render()
renderer.GetActiveCamera().Azimuth(-50)
renderer.GetActiveCamera().Roll(70)
renWin.Render()
# --- end of script --
