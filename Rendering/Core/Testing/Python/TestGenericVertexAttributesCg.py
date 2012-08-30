#!/usr/bin/env python

# tests the support to pass generic vertex attributes to be used in Cg shaders.
xmlMaterial = <?xml version="1.0" encoding="UTF-8"?>
<Material name="GenericAttributes1">
  <Shader
    scope="Vertex"
    name="VertexShader"
    location="Inline"
    language="Cg"
    entry="main">
      <MatrixUniform name="ModelViewProj"
        type="State"
        number_of_elements="2"
        value="CG_GL_MODELVIEW_PROJECTION_MATRIX CG_GL_MATRIX_IDENTITY" />
      <MatrixUniform name="ModelViewIT"
        type="State"
        number_of_elements="2"
        value="CG_GL_MODELVIEW_MATRIX CG_GL_MATRIX_INVERSE_TRANSPOSE" />

      struct appin
      {
          float4 Position : POSITION;
          float3 Normal   : NORMAL;
      };

      // define outputs from vertex shader
      struct vertout
      {
          float4 HPosition : POSITION;
          float4 Color0    : COLOR0;
      };

      vertout main(appin IN,
                   uniform float4x4 ModelViewProj,
                   uniform float4x4 ModelViewIT)
      {
        vertout OUT;

        // transform vertex position into homogenous clip-space
        OUT.HPosition = mul(ModelViewProj, IN.Position);

        OUT.Color0.xyz = normalize(IN.Normal);
        OUT.Color0.a = 1.0;
        return OUT;
      }
  </Shader>
</Material>

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
# Map PointData.BrownianVectors (all 3 components) to IN.Normal
mapper.MapDataArrayToVertexAttribute("IN.Normal","BrownianVectors",0,-1)
renderer.AddActor(actor)
renderer.SetBackground(0.5,0.5,0.5)
renWin.Render()
renderer.GetActiveCamera().Azimuth(-50)
renderer.GetActiveCamera().Roll(70)
renWin.Render()
# --- end of script --
