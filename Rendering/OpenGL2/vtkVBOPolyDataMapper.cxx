/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVBOPolyDataMapper.h"

#include "vtkglVBOHelper.h"

#include "vtkCommand.h"
#include "vtkCamera.h"
#include "vtkTransform.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"
#include "vtkPolyData.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkPointData.h"
#include "vtkCellArray.h"
#include "vtkVector.h"
#include "vtkProperty.h"
#include "vtkMatrix3x3.h"
#include "vtkMatrix4x4.h"
#include "vtkLookupTable.h"
#include "vtkCellData.h"
#include "vtkPolyDataNormals.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"

#include "vtkLight.h"
#include "vtkLightCollection.h"

// Bring in our shader symbols.
#include "vtkglPolyDataVSLightKit.h"
#include "vtkglPolyDataVSHeadlight.h"
#include "vtkglPolyDataVSPositionalLights.h"
#include "vtkglVertexShader.h"
#include "vtkglPolyDataFS.h"

using vtkgl::replace;

class vtkVBOPolyDataMapper::Private
{
public:
  // The VBO and its layout.
  vtkgl::BufferObject vbo;
  vtkgl::VBOLayout layout;

  // Structures for the various cell types we render.
  vtkgl::CellBO points;
  vtkgl::CellBO lines;
  vtkgl::CellBO tris;
  vtkgl::CellBO triStrips;

  // Array of colors, along with the number of components.
  std::vector<unsigned char> colors;
  unsigned char colorComponents;
  bool colorAttributes;

  bool buidNormals;
  int interpolation;

  vtkTimeStamp propertiesTime;

  Private() : colorAttributes(false), buidNormals(true)
  {
  }

  void SetMaterialUniforms(vtkgl::ShaderProgram &program, vtkProperty *property)
  {
    // Query the actor for some of the properties that can be applied.
    float opacity = static_cast<float>(property->GetOpacity());
    double *color = property->GetDiffuseColor();
    vtkgl::Vector3ub diffuseColor(
          static_cast<unsigned char>(color[0] * 255.0),
          static_cast<unsigned char>(color[1] * 255.0),
          static_cast<unsigned char>(color[2] * 255.0));
    program.SetUniformValue("opacity", opacity);
    program.SetUniformValue("diffuseColor", diffuseColor);
    program.SetUniformValue("pointSize", property->GetPointSize());
    glLineWidth(property->GetLineWidth());
  }

  void SetCameraUniforms(vtkgl::ShaderProgram &program, vtkRenderer *ren,
                         vtkActor *actor)
  {
    // pass down the various model and camera transformations
    vtkCamera *cam = ren->GetActiveCamera();
    vtkTransform* viewTF = cam->GetModelViewTransformObject();
    // compute the combined ModelView matrix and send it down to save time in the shader
    vtkNew<vtkMatrix4x4> tmpMat;
    vtkMatrix4x4::Multiply4x4(viewTF->GetMatrix(), actor->GetMatrix(),
                              tmpMat.Get());
    tmpMat->Transpose();
    program.SetUniformValue("MCVCMatrix", tmpMat.Get());
    tmpMat->DeepCopy(cam->GetProjectionTransformMatrix(ren));
    program.SetUniformValue("VCDCMatrix", tmpMat.Get());
  }
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkVBOPolyDataMapper)

//-----------------------------------------------------------------------------
vtkVBOPolyDataMapper::vtkVBOPolyDataMapper()
  : Internal(new Private), UsingScalarColoring(false)
{
}

//-----------------------------------------------------------------------------
vtkVBOPolyDataMapper::~vtkVBOPolyDataMapper()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void vtkVBOPolyDataMapper::ReleaseGraphicsResources(vtkWindow*)
{
  // FIXME: Implement resource release.
}

//-----------------------------------------------------------------------------
void vtkVBOPolyDataMapper::UpdateShader(vtkRenderer* ren, vtkActor *vtkNotUsed(actor))
{
  // first see if anything has changed, if not, just return
  // do this by checking lightcollection mtime

  // consider the lighting complexity to determine which case applies
  // simple headlight, Light Kit, the whole feature set of VTK
  int lightComplexity = 1;
  int numberOfLights = 0;
  vtkLightCollection *lc = ren->GetLights();
  vtkLight *light;

  vtkCollectionSimpleIterator sit;
  for(lc->InitTraversal(sit);
      (light = lc->GetNextLight(sit)); )
    {
    float status = light->GetSwitch();
    if (status > 0.0)
      {
      numberOfLights++;
      }

    if (lightComplexity == 1
        && (numberOfLights > 1
          || light->GetIntensity() != 1.0
          || light->GetLightType() != VTK_LIGHT_TYPE_HEADLIGHT))
      {
        lightComplexity = 2;
      }
    if (lightComplexity < 3
        && (light->GetPositional()))
      {
        lightComplexity = 3;
        break;
      }
    }

  vtkgl::CellBO &tris = this->Internal->tris;

  // pick which shader code to use based on above factors
  switch (lightComplexity)
    {
    case 1:
        tris.fsFile = vtkglPolyDataFS;
        tris.vsFile = vtkglPolyDataVSHeadlight;
  //        tris.vsFile = vtkglPolyDataVSPositionalLights;
      break;
    case 2:
        tris.fsFile = vtkglPolyDataFS;
  //        tris.vsFile = vtkglPolyDataVSHeadlight;
        tris.vsFile = vtkglPolyDataVSLightKit;
  //        tris.vsFile = vtkglPolyDataVSPositionalLights;
      break;
    case 3:
        tris.fsFile = vtkglPolyDataFS;
        tris.vsFile = vtkglPolyDataVSPositionalLights;
      break;
    }

  // compile and link the shader program if it has changed
  // eventually use some sort of caching here
  if (tris.vs.GetType() == vtkgl::Shader::Unknown ||
      this->Internal->propertiesTime > tris.buildTime)
    {
    // Build our shader if necessary.
    std::string vertexShaderSource = tris.vsFile;
    if (this->Internal->colorAttributes)
      {
      vertexShaderSource = replace(vertexShaderSource,
                                   "//VTK::Color::Dec",
                                   "attribute vec4 diffuseColor;");
      }
    else
      {
      vertexShaderSource = replace(vertexShaderSource,
                                   "//VTK::Color::Dec",
                                   "uniform vec3 diffuseColor;");
      }
    cout << "VS: " << vertexShaderSource << endl;

    tris.vs.SetSource(vertexShaderSource);
    tris.vs.SetType(vtkgl::Shader::Vertex);
    tris.fs.SetSource(tris.fsFile);
    tris.fs.SetType(vtkgl::Shader::Fragment);
    if (!tris.vs.Compile())
      {
      vtkErrorMacro(<< tris.vs.GetError());
      }
    if (!tris.fs.Compile())
      {
      vtkErrorMacro(<< tris.fs.GetError());
      }
    if (!tris.program.AttachShader(tris.vs))
      {
      vtkErrorMacro(<< this->Internal->tris.program.GetError());
      }
    if (!tris.program.AttachShader(tris.fs))
      {
      vtkErrorMacro(<< tris.program.GetError());
      }
    if (!tris.program.Link())
      {
      vtkErrorMacro(<< "Links failed: " << tris.program.GetError());
      }
    tris.buildTime.Modified();
    }
  // Now to update the VAO too, if necessary.
  vtkgl::VBOLayout &layout = this->Internal->layout;
  if (tris.indexCount && this->VBOUpdateTime > tris.attributeUpdateTime)
    {
    tris.program.Bind();
    tris.vao.Bind();
    if (!tris.vao.AddAttributeArray(tris.program, this->Internal->vbo,
                                    "vertexMC", layout.VertexOffset,
                                    layout.Stride, VTK_FLOAT, 3, false))
      {
      vtkErrorMacro(<< "Error setting 'vertexMC' in triangle VAO.");
      }
    if (layout.VertexOffset != layout.NormalOffset)
      {
      if (!tris.vao.AddAttributeArray(tris.program, this->Internal->vbo,
                                      "normalMC", layout.NormalOffset,
                                      layout.Stride, VTK_FLOAT, 3, false))
        {
        vtkErrorMacro(<< "Error setting 'normalMC' in triangle VAO.");
        }
      }
    if (layout.ColorComponents != 0)
      {
      if (!tris.vao.AddAttributeArray(tris.program, this->Internal->vbo,
                                      "diffuseColor", layout.ColorOffset,
                                      layout.Stride, VTK_UNSIGNED_CHAR,
                                      layout.ColorComponents, true))
        {
        vtkErrorMacro(<< "Error setting 'diffuseColor' in triangle VAO.");
        }
      }
    tris.attributeUpdateTime.Modified();
    }
}

void vtkVBOPolyDataMapper::UpdateVertexShader(vtkRenderer *, vtkActor *)
{
  // Compile and link the shader program if it has changed.
  // FIXME: Use caching for shaders/programs.
  vtkgl::CellBO &points = this->Internal->points;
  if (points.vs.GetType() == vtkgl::Shader::Unknown)
    {
    points.vs.SetType(vtkgl::Shader::Vertex);
    points.fs.SetType(vtkgl::Shader::Fragment);
    // Build our shader if necessary.
    std::string vertexShaderSource = vtkglVertexShader;
    if (this->Internal->colorAttributes)
      {
      vertexShaderSource = replace(vertexShaderSource,
                                   "//VTK::Color::Dec",
                                   "attribute vec4 diffuseColor;");
      }
    else
      {
      vertexShaderSource = replace(vertexShaderSource,
                                   "//VTK::Color::Dec",
                                   "uniform vec3 diffuseColor;");
      }
    cout << "VS: " << vertexShaderSource << endl;

    points.vs.SetType(vtkgl::Shader::Vertex);
    points.vs.SetSource(vertexShaderSource);
    points.fs.SetType(vtkgl::Shader::Fragment);
    points.fs.SetSource(vtkglPolyDataFS);
    if (!points.fs.Compile())
      {
      vtkErrorMacro(<< points.fs.GetError());
      }
    if (!points.vs.Compile())
      {
      vtkErrorMacro(<< points.vs.GetError());
      }

    if (!points.program.AttachShader(points.vs))
      {
      vtkErrorMacro(<< points.program.GetError());
      }
    if (!points.program.AttachShader(points.fs))
      {
      vtkErrorMacro(<< points.program.GetError());
      }
    if (!points.program.Link())
      {
      vtkErrorMacro(<< points.program.GetError());
      }
    }
  // Now to update the VAO too, if necessary.
  vtkgl::VBOLayout &layout = this->Internal->layout;
  if (points.indexCount && this->VBOUpdateTime > points.attributeUpdateTime)
    {
    points.program.Bind();
    points.vao.Bind();
    if (!points.vao.AddAttributeArray(points.program, this->Internal->vbo,
                                      "vertexMC", layout.VertexOffset,
                                       layout.Stride, VTK_FLOAT, 3, false))
      {
      vtkErrorMacro(<< "Error setting 'vertexMC' in VAO.");
      }
    // Do we want normals in lines?
    /* if (layout.VertexOffset != layout.NormalOffset)
      {
      if (!points.vao.AddAttributeArray(points.program, this->Internal->vbo,
                                        "normalMC", layout.NormalOffset,
                                        layout.Stride, VTK_FLOAT, 3, false))
        {
        vtkErrorMacro(<< "Error setting 'normalMC' in VAO.");
        }
      } */
    if (layout.ColorComponents != 0)
      {
      if (!points.vao.AddAttributeArray(points.program, this->Internal->vbo,
                                        "diffuseColor", layout.ColorOffset,
                                        layout.Stride, VTK_UNSIGNED_CHAR,
                                        layout.ColorComponents, true))
        {
        vtkErrorMacro(<< "Error setting 'diffuseColor' in VAO.");
        }
      }
    points.attributeUpdateTime.Modified();
    }
}

void vtkVBOPolyDataMapper::UpdateLineShader(vtkRenderer *, vtkActor *)
{
  // Compile and link the shader program if it has changed.
  // FIXME: Use caching for shaders/programs.
  vtkgl::CellBO &lines = this->Internal->lines;
  if (lines.vs.GetType() == vtkgl::Shader::Unknown)
    {
    lines.vs.SetType(vtkgl::Shader::Vertex);
    lines.fs.SetType(vtkgl::Shader::Fragment);
    // Build our shader if necessary.
    std::string vertexShaderSource = vtkglVertexShader;
    if (this->Internal->colorAttributes)
      {
      vertexShaderSource = replace(vertexShaderSource,
                                   "//VTK::Color::Dec",
                                   "attribute vec4 diffuseColor;");
      }
    else
      {
      vertexShaderSource = replace(vertexShaderSource,
                                   "//VTK::Color::Dec",
                                   "uniform vec3 diffuseColor;");
      }
    cout << "VS: " << vertexShaderSource << endl;

    lines.vs.SetType(vtkgl::Shader::Vertex);
    lines.vs.SetSource(vertexShaderSource);
    lines.fs.SetType(vtkgl::Shader::Fragment);
    lines.fs.SetSource(vtkglPolyDataFS);
    if (!lines.fs.Compile())
      {
      vtkErrorMacro(<< lines.fs.GetError());
      }
    if (!lines.vs.Compile())
      {
      vtkErrorMacro(<< lines.vs.GetError());
      }

    if (!lines.program.AttachShader(lines.vs))
      {
      vtkErrorMacro(<< lines.program.GetError());
      }
    if (!lines.program.AttachShader(lines.fs))
      {
      vtkErrorMacro(<< lines.program.GetError());
      }
    if (!lines.program.Link())
      {
      vtkErrorMacro(<< lines.program.GetError());
      }
    }
  // Now to update the VAO too, if necessary.
  vtkgl::VBOLayout &layout = this->Internal->layout;
  if (lines.indexCount && this->VBOUpdateTime > lines.attributeUpdateTime)
    {
    lines.program.Bind();
    lines.vao.Bind();
    if (!lines.vao.AddAttributeArray(lines.program, this->Internal->vbo,
                                     "vertexMC", layout.VertexOffset,
                                     layout.Stride, VTK_FLOAT, 3, true))
      {
      vtkErrorMacro(<< "Error setting 'vertexMC' in VAO.");
      }
    // Do we want normals in lines?
    /* if (layout.VertexOffset != layout.NormalOffset)
      {
      if (!lines.vao.AddAttributeArray(lines.program, this->Internal->vbo,
                                       "normalMC", layout.NormalOffset,
                                       layout.Stride, VTK_FLOAT, 3, false))
        {
        vtkErrorMacro(<< "Error setting 'normalMC' in VAO.");
        }
      } */
    if (layout.ColorComponents != 0)
      {
      if (!lines.vao.AddAttributeArray(lines.program, this->Internal->vbo,
                                       "diffuseColor", layout.ColorOffset,
                                       layout.Stride, VTK_UNSIGNED_CHAR,
                                       layout.ColorComponents, true))
        {
        vtkErrorMacro(<< "Error setting 'diffuseColor' in VAO.");
        }
      }
    lines.attributeUpdateTime.Modified();
    }
}

//-----------------------------------------------------------------------------
void vtkVBOPolyDataMapper::SetLightingShaderParameters(vtkRenderer* ren, vtkActor *vtkNotUsed(actor))
{
  // for headlight there are no lighting parameters
  if (this->Internal->tris.vsFile == vtkglPolyDataVSHeadlight)
    {
    return;
    }

  // for lightkit case there are some parameters to set
  vtkCamera *cam = ren->GetActiveCamera();
  vtkTransform* viewTF = cam->GetModelViewTransformObject();

  // bind some light settings
  int numberOfLights = 0;
  vtkLightCollection *lc = ren->GetLights();
  vtkLight *light;

  vtkCollectionSimpleIterator sit;
  float lightColor[6][3];
  float lightDirection[6][3];
  for(lc->InitTraversal(sit);
      (light = lc->GetNextLight(sit)); )
    {
    float status = light->GetSwitch();
    if (status > 0.0)
      {
      double *dColor = light->GetDiffuseColor();
      double intensity = light->GetIntensity();
      lightColor[numberOfLights][0] = dColor[0] * intensity;
      lightColor[numberOfLights][1] = dColor[1] * intensity;
      lightColor[numberOfLights][2] = dColor[2] * intensity;
      // get required info from light
      double *lfp = light->GetTransformedFocalPoint();
      double *lp = light->GetTransformedPosition();
      double lightDir[3];
      vtkMath::Subtract(lfp,lp,lightDir);
      vtkMath::Normalize(lightDir);
      double *tDir = viewTF->TransformNormal(lightDir);
      lightDirection[numberOfLights][0] = tDir[0];
      lightDirection[numberOfLights][1] = tDir[1];
      lightDirection[numberOfLights][2] = tDir[2];
      numberOfLights++;
      }
    }
  vtkgl::ShaderProgram &program = this->Internal->tris.program;

  program.SetUniformValue("lightColor", numberOfLights, lightColor);
  program.SetUniformValue("lightDirectionVC", numberOfLights, lightDirection);
  program.SetUniformValue("numberOfLights", numberOfLights);

  if (this->Internal->tris.vsFile == vtkglPolyDataVSLightKit)
    {
    return;
    }

  // if positional lights pass down more parameters
  float lightAttenuation[6][3];
  float lightPosition[6][3];
  float lightConeAngle[6];
  float lightExponent[6];
  int lightPositional[6];
  numberOfLights = 0;
  for(lc->InitTraversal(sit);
      (light = lc->GetNextLight(sit)); )
    {
    float status = light->GetSwitch();
    if (status > 0.0)
      {
      double *attn = light->GetAttenuationValues();
      lightAttenuation[numberOfLights][0] = attn[0];
      lightAttenuation[numberOfLights][1] = attn[1];
      lightAttenuation[numberOfLights][2] = attn[2];
      lightExponent[numberOfLights] = light->GetExponent();
      lightConeAngle[numberOfLights] = light->GetConeAngle();
      double *lp = light->GetTransformedPosition();
      lightPosition[numberOfLights][0] = lp[0];
      lightPosition[numberOfLights][1] = lp[1];
      lightPosition[numberOfLights][2] = lp[2];
      lightPositional[numberOfLights] = light->GetPositional();
      numberOfLights++;
      }
    }
  program.SetUniformValue("lightAttenuation", numberOfLights, lightAttenuation);
  program.SetUniformValue("lightPositional", numberOfLights, lightPositional);
  program.SetUniformValue("lightPositionWC", numberOfLights, lightPosition);
  program.SetUniformValue("lightExponent", numberOfLights, lightExponent);
  program.SetUniformValue("lightConeAngle", numberOfLights, lightConeAngle);
}

//-----------------------------------------------------------------------------
void vtkVBOPolyDataMapper::SetCameraShaderParameters(vtkRenderer* ren, vtkActor *actor)
{
  vtkgl::ShaderProgram &program = this->Internal->tris.program;
  // pass down the various model and camera transformations
  vtkCamera *cam = ren->GetActiveCamera();
  // really just view  matrix in spite of it's name
  vtkTransform* viewTF = cam->GetModelViewTransformObject();
  program.SetUniformValue("WCVCMatrix", viewTF->GetMatrix());

  // set the MCWC matrix
  program.SetUniformValue("MCWCMatrix", actor->GetMatrix());

  // compute the combined ModelView matrix and send it down to save time in the shader
  vtkMatrix4x4 *tmpMat = vtkMatrix4x4::New();
  vtkMatrix4x4::Multiply4x4(viewTF->GetMatrix(), actor->GetMatrix(), tmpMat);
  tmpMat->Transpose();
  program.SetUniformValue("MCVCMatrix", tmpMat);

  // set the normal matrix and send it down
  // (make this a function in camera at some point returning a 3x3)
  tmpMat->DeepCopy(cam->GetViewTransformMatrix());
  vtkMatrix3x3 *tmpMat3d = vtkMatrix3x3::New();
  for(int i = 0; i < 3; ++i)
    {
    for (int j = 0; j < 3; ++j)
      {
        tmpMat3d->SetElement(i,j,tmpMat->GetElement(i,j));
      }
    }
  tmpMat3d->Invert();
  program.SetUniformValue("normalMatrix", tmpMat3d);

  tmpMat->DeepCopy(cam->GetProjectionTransformMatrix(ren));
  program.SetUniformValue("VCDCMatrix", tmpMat);

  tmpMat->Delete();
  tmpMat3d->Delete();
}

//-----------------------------------------------------------------------------
void vtkVBOPolyDataMapper::SetPropertyShaderParameters(vtkRenderer*,
                                                       vtkActor *actor)
{
  // Query the actor for some of the properties that can be applied.
  float opacity = static_cast<float>(actor->GetProperty()->GetOpacity());
  double *aColor = actor->GetProperty()->GetAmbientColor();
  double aIntensity = actor->GetProperty()->GetAmbient();  // ignoring renderer ambient
  vtkgl::Vector3ub ambientColor(static_cast<unsigned char>(aColor[0] * aIntensity * 255.0),
                         static_cast<unsigned char>(aColor[1] * aIntensity * 255.0),
                         static_cast<unsigned char>(aColor[2] * aIntensity * 255.0));
  double *dColor = actor->GetProperty()->GetDiffuseColor();
  double dIntensity = actor->GetProperty()->GetDiffuse();
  vtkgl::Vector3ub diffuseColor(static_cast<unsigned char>(dColor[0] * dIntensity * 255.0),
                         static_cast<unsigned char>(dColor[1] * dIntensity * 255.0),
                         static_cast<unsigned char>(dColor[2] * dIntensity * 255.0));
  double *sColor = actor->GetProperty()->GetSpecularColor();
  double sIntensity = actor->GetProperty()->GetSpecular();
  vtkgl::Vector3ub specularColor(static_cast<unsigned char>(sColor[0] * sIntensity * 255.0),
                         static_cast<unsigned char>(sColor[1] * sIntensity * 255.0),
                         static_cast<unsigned char>(sColor[2] * sIntensity * 255.0));
  float specularPower = actor->GetProperty()->GetSpecularPower();

  vtkgl::ShaderProgram &program = this->Internal->tris.program;
  program.SetUniformValue("opacity", opacity);
  program.SetUniformValue("ambientColor", ambientColor);
  program.SetUniformValue("diffuseColor", diffuseColor);
  program.SetUniformValue("specularColor", specularColor);
  program.SetUniformValue("specularPower", specularPower);
}

//-----------------------------------------------------------------------------
void vtkVBOPolyDataMapper::RenderPiece(vtkRenderer* ren, vtkActor *actor)
{
  vtkDataObject *input= this->GetInputDataObject(0, 0);

  // Make sure that we have been properly initialized.
  if (ren->GetRenderWindow()->CheckAbortStatus())
    {
    return;
    }

  if (input == NULL)
    {
    vtkErrorMacro(<< "No input!");
    return;
    }
  else
    {
    this->InvokeEvent(vtkCommand::StartEvent,NULL);
    if (!this->Static)
      {
      this->GetInputAlgorithm()->Update();
      }
    this->InvokeEvent(vtkCommand::EndEvent,NULL);
    }

  this->TimeToDraw = 0.0;

  // Update the VBO if needed.
  if (this->VBOUpdateTime < this->GetMTime() ||
      this->VBOUpdateTime < actor->GetProperty()->GetMTime())
    {
    this->UpdateVBO(actor);
    this->VBOUpdateTime.Modified();
    }

  // Bind the VBO, this is shared between the different primitive/cell types.
  this->Internal->vbo.Bind();
  vtkgl::VBOLayout &layout = this->Internal->layout;

  if (this->Internal->tris.indexCount)
    {
    // First we do the triangles, update the shader, set uniforms, etc.
    this->UpdateShader(ren, actor);
    if (!this->Internal->tris.program.Bind())
      {
      vtkErrorMacro(<< this->Internal->tris.program.GetError());
      return;
      }

    this->SetLightingShaderParameters(ren, actor);
    this->SetPropertyShaderParameters(ren, actor);
    this->SetCameraShaderParameters(ren, actor);

    this->Internal->tris.vao.Bind();

    // Render the VBO contents as appropriate, I think we really need separate
    // shaders for triangles, lines and points too...
    this->Internal->tris.ibo.Bind();

    if (actor->GetProperty()->GetRepresentation() == VTK_SURFACE)
      {
      glDrawRangeElements(GL_TRIANGLES, 0,
                          static_cast<GLuint>(layout.VertexCount - 1),
                          static_cast<GLsizei>(this->Internal->tris.indexCount),
                          GL_UNSIGNED_INT,
                          reinterpret_cast<const GLvoid *>(NULL));
      }
    else if (actor->GetProperty()->GetRepresentation() == VTK_WIREFRAME)
      {
#if 1
      glMultiDrawElements(GL_LINE_LOOP,
                        (GLsizei *)(&this->Internal->tris.elementsArray[0]),
                        GL_UNSIGNED_INT,
                        reinterpret_cast<const GLvoid **>(&(this->Internal->tris.offsetArray[0])),
                        this->Internal->tris.offsetArray.size());
#else
      for (int eCount = 0; eCount < this->Internal->tris.offsetArray.size(); ++eCount)
        {
        glDrawElements(GL_LINE_LOOP,
          this->Internal->tris.elementsArray[eCount],
          GL_UNSIGNED_INT,
          (GLvoid *)(this->Internal->tris.offsetArray[eCount]));
        }
#endif
      }
    this->Internal->tris.vao.Release();
    this->Internal->tris.ibo.Release();
    this->Internal->tris.program.Release();
    }

  if (this->Internal->triStrips.indexCount)
    {
    // Use the tris shader program/VAO, but triStrips ibo.
    this->UpdateShader(ren, actor);
    if (!this->Internal->tris.program.Bind())
      {
      vtkErrorMacro(<< this->Internal->tris.program.GetError());
      return;
      }

    this->SetLightingShaderParameters(ren, actor);
    this->SetPropertyShaderParameters(ren, actor);
    this->SetCameraShaderParameters(ren, actor);

    this->Internal->tris.vao.Bind();

    // Render the triangle strips, iterating over each strip.
    this->Internal->triStrips.ibo.Bind();
    for (int eCount = 0; eCount < this->Internal->triStrips.offsetArray.size(); ++eCount)
      {
      glDrawElements(GL_TRIANGLE_STRIP,
        this->Internal->triStrips.elementsArray[eCount],
        GL_UNSIGNED_INT,
        (GLvoid *)(this->Internal->triStrips.offsetArray[eCount]));
      }
    this->Internal->tris.vao.Release();
    this->Internal->triStrips.ibo.Release();
    this->Internal->tris.program.Release();
    }

  if (this->Internal->lines.indexCount)
    {
    // Update/build the shader.
    this->UpdateLineShader(ren, actor);
    if (!this->Internal->lines.program.Bind())
      {
      vtkErrorMacro(<< this->Internal->lines.program.GetError());
      return;
      }

    this->Internal->SetMaterialUniforms(this->Internal->lines.program,
                                        actor->GetProperty());
    this->Internal->SetCameraUniforms(this->Internal->lines.program, ren, actor);

    this->Internal->lines.vao.Bind();
    this->Internal->lines.ibo.Bind();
    for (int eCount = 0; eCount < this->Internal->lines.offsetArray.size(); ++eCount)
      {
      glDrawElements(GL_LINE_STRIP,
        this->Internal->lines.elementsArray[eCount],
        GL_UNSIGNED_INT,
        (GLvoid *)(this->Internal->lines.offsetArray[eCount]));
      }
    this->Internal->lines.vao.Release();
    this->Internal->lines.ibo.Release();
    this->Internal->lines.program.Release();
    }

  if (this->Internal->points.indexCount)
    {
    // Update/build the shader.
    this->UpdateVertexShader(ren, actor);
    if (!this->Internal->points.program.Bind())
      {
      vtkErrorMacro(<< this->Internal->points.program.GetError());
      return;
      }

    this->Internal->SetMaterialUniforms(this->Internal->points.program,
                                        actor->GetProperty());
    this->Internal->SetCameraUniforms(this->Internal->points.program, ren, actor);

    this->Internal->points.vao.Bind();
    this->Internal->points.ibo.Bind();
    glDrawRangeElements(GL_POINTS, 0,
                        static_cast<GLuint>(layout.VertexCount - 1),
                        static_cast<GLsizei>(this->Internal->points.indexCount),
                        GL_UNSIGNED_INT,
                        reinterpret_cast<const GLvoid *>(NULL));
    this->Internal->points.vao.Release();
    this->Internal->points.ibo.Release();
    this->Internal->points.program.Release();
    }

  this->Internal->vbo.Release();

  // If the timer is not accurate enough, set it to a small
  // time so that it is not zero
  if (this->TimeToDraw == 0.0)
    {
    this->TimeToDraw = 0.0001;
    }

  this->UpdateProgress(1.0);
}

//-------------------------------------------------------------------------
void vtkVBOPolyDataMapper::ComputeBounds()
{
  if (!this->GetInput())
    {
    vtkMath::UninitializeBounds(this->Bounds);
    return;
    }
  this->GetInput()->GetBounds(this->Bounds);
}

//-------------------------------------------------------------------------
void vtkVBOPolyDataMapper::UpdateVBO(vtkActor *act)
{
  vtkPolyData *poly = this->GetInput();
  if (poly == NULL)// || !poly->GetPointData()->GetNormals())
    {
    return;
    }

  vtkSmartPointer<vtkDataArray> n;

  // This replicates how the painter decided on normal generation.
  int interpolation = act->GetProperty()->GetInterpolation();
  bool buildNormals = this->Internal->buidNormals;
  if (buildNormals)
    {
    buildNormals = ((poly->GetPointData()->GetNormals() && interpolation != VTK_FLAT) ||
                    poly->GetCellData()->GetNormals()) ? false : true;

    if (buildNormals)
      {
      vtkNew<vtkPolyDataNormals> computeNormals;
      computeNormals->SetInputData(poly);
      computeNormals->SplittingOff();
      computeNormals->Update();
      n = computeNormals->GetOutput()->GetPointData()->GetNormals();
      }
    else
      {
      n = poly->GetPointData()->GetNormals();
      }
    }

  // Mark our properties as updated.
  this->Internal->propertiesTime.Modified();

  bool colorAttributes = false;
  this->Internal->colorComponents = 0;
  if (this->ScalarVisibility)
    {
    // We must figure out how the scalars should be mapped to the polydata.
    this->MapScalars(NULL, 1.0, false, poly);
    if (this->Internal->colorComponents == 3 ||
        this->Internal->colorComponents == 4)
      {
      this->Internal->colorAttributes = colorAttributes = true;
      cout << "Scalar colors: "
           << this->Internal->colors.size() / this->Internal->colorComponents
           << " with " << int(this->Internal->colorComponents) << " components." <<  endl;
      }
    }

  // Create a mesh packed with vertex, normal and possibly color.
  if (n && n->GetNumberOfTuples() != poly->GetNumberOfPoints())
    {
    vtkErrorMacro(<< "Polydata without enough normals for all points. "
                  << poly->GetNumberOfPoints() - n->GetNumberOfTuples());
    }

  // Iterate through all of the different types in the polydata, building VBOs
  // and IBOs as appropriate for each type.
  this->Internal->layout =
    CreateVBO(poly->GetPoints(),
              n,
              this->Internal->colorComponents ? &this->Internal->colors[0] : NULL,
              this->Internal->colorComponents,
              this->Internal->vbo);

  // create the IBOs
  // for polys if we are wireframe handle it with multiindiex buffer
  vtkgl::CellBO &tris = this->Internal->tris;
  if (act->GetProperty()->GetRepresentation() == VTK_SURFACE)
    {
    tris.indexCount = CreateTriangleIndexBuffer(poly->GetPolys(),
                                                tris.ibo,
                                                poly->GetPoints());
    }
  else if (act->GetProperty()->GetRepresentation() == VTK_WIREFRAME)
    {
    tris.indexCount = CreateMultiIndexBuffer(poly->GetPolys(),
                                             tris.ibo,
                                             tris.offsetArray,
                                             tris.elementsArray);
    }

  this->Internal->points.indexCount = CreatePointIndexBuffer(poly->GetVerts(),
                                                        this->Internal->points.ibo);

  this->Internal->triStrips.indexCount = CreateMultiIndexBuffer(poly->GetStrips(),
                         this->Internal->triStrips.ibo,
                         this->Internal->triStrips.offsetArray,
                         this->Internal->triStrips.elementsArray);

  this->Internal->lines.indexCount = CreateMultiIndexBuffer(poly->GetLines(),
                         this->Internal->lines.ibo,
                         this->Internal->lines.offsetArray,
                         this->Internal->lines.elementsArray);
}

//-----------------------------------------------------------------------------
bool vtkVBOPolyDataMapper::GetIsOpaque()
{
  // Straight copy of what the vtkPainterPolyDataMapper was doing.
  if (this->ScalarVisibility &&
    this->ColorMode == VTK_COLOR_MODE_DEFAULT)
    {
    vtkPolyData* input =
      vtkPolyData::SafeDownCast(this->GetInputDataObject(0, 0));
    if (input)
      {
      int cellFlag;
      vtkDataArray* scalars = this->GetScalars(input,
        this->ScalarMode, this->ArrayAccessMode, this->ArrayId,
        this->ArrayName, cellFlag);
      if (scalars && scalars->IsA("vtkUnsignedCharArray") &&
        (scalars->GetNumberOfComponents() ==  4 /*(RGBA)*/ ||
         scalars->GetNumberOfComponents() == 2 /*(LuminanceAlpha)*/))
        {
        vtkUnsignedCharArray* colors =
          static_cast<vtkUnsignedCharArray*>(scalars);
        if ((colors->GetNumberOfComponents() == 4 && colors->GetValueRange(3)[0] < 255) ||
          (colors->GetNumberOfComponents() == 2 && colors->GetValueRange(1)[0] < 255))
          {
          // If the opacity is 255, despite the fact that the user specified
          // RGBA, we know that the Alpha is 100% opaque. So treat as opaque.
          return false;
          }
        }
      }
    }
  return this->Superclass::GetIsOpaque();
}

namespace
{
inline void vtkMultiplyColorsWithAlpha(vtkDataArray* array)
{
  vtkUnsignedCharArray* colors = vtkUnsignedCharArray::SafeDownCast(array);
  if (!colors || colors->GetNumberOfComponents() != 4)
    {
    return;
    }
  unsigned char* ptr = colors->GetPointer(0);
  vtkIdType numValues =
      colors->GetNumberOfTuples() * colors->GetNumberOfComponents();
  if (numValues <= 4)
    {
    return;
    }
  for (vtkIdType cc = 0; cc < numValues; cc += 4, ptr += 4)
    {
    double alpha = (0x0ff & ptr[3]) / 255.0;
    ptr[0] = static_cast<unsigned char>(0x0ff &
                                        static_cast<int>((0x0ff &
                                                          ptr[0]) * alpha));
    ptr[1] = static_cast<unsigned char>(0x0ff &
                                        static_cast<int>((0x0ff &
                                                          ptr[1]) * alpha));
    ptr[2] = static_cast<unsigned char>(0x0ff &
                                        static_cast<int>((0x0ff &
                                                          ptr[2]) * alpha));
    }
}
}

//-----------------------------------------------------------------------------
// This method has the same functionality as the old vtkMapper::MapScalars.
void vtkVBOPolyDataMapper::MapScalars(vtkDataSet*, double alpha,
                                      bool multiplyWithAlpha, vtkDataSet* input)
{
  int cellFlag;
  double origAlpha;
  vtkDataArray* scalars = vtkAbstractMapper::GetScalars(input,
    this->ScalarMode, this->ArrayAccessMode, this->ArrayId,
    this->ArrayName, cellFlag);

  int arraycomponent = this->ArrayComponent;
  // This is for a legacy feature: selection of the array component to color by
  // from the mapper.  It is now in the lookuptable.  When this feature
  // is removed, we can remove this condition.
  if (scalars == 0 || scalars->GetNumberOfComponents() <= this->ArrayComponent)
    {
    arraycomponent = 0;
    }

  if (!this->ScalarVisibility || scalars == 0 || input == 0)
    {
    return;
    }

  // Let subclasses know that scalar coloring was employed in the current pass.
  this->UsingScalarColoring = true;
  if (this->ColorTextureMap)
    {
    /// FIXME: Implement, or move this.
    // Implies that we have verified that we must use texture map for scalar
    // coloring. Just create texture coordinates for the input dataset.
    //this->MapScalarsToTexture(output, scalars, input);
    return;
    }

  vtkScalarsToColors* lut = 0;
  // Get the lookup table.
  if (scalars->GetLookupTable())
    {
    lut = scalars->GetLookupTable();
    }
  else
    {
    lut = this->GetLookupTable();
    lut->Build();
    }

  if (!this->UseLookupTableScalarRange)
    {
    lut->SetRange(this->ScalarRange);
    }

  // The LastUsedAlpha checks ensures that opacity changes are reflected
  // correctly when this->MapScalars(..) is called when iterating over a
  // composite dataset.
  /*if (colors &&
    this->LastUsedAlpha == alpha &&
    this->LastUsedMultiplyWithAlpha == multiplyWithAlpha)
    {
    if (this->GetMTime() < colors->GetMTime() &&
      input->GetMTime() < colors->GetMTime() &&
      lut->GetMTime() < colors->GetMTime())
      {
      // using old colors.
      return;
      }
    }*/

  // Get rid of old colors.
  vtkDataArray *colors = 0;
  origAlpha = lut->GetAlpha();
  lut->SetAlpha(alpha);
  colors = lut->MapScalars(scalars, this->ColorMode, arraycomponent);
  lut->SetAlpha(origAlpha);
  if (multiplyWithAlpha)
    {
    // It is possible that the LUT simply returns the scalars as the
    // colors. In which case, we allocate a new array to ensure
    // that we don't modify the array in the input.
    if (scalars == colors)
      {
      // Since we will be changing the colors array
      // we create a copy.
      colors->Delete();
      colors = scalars->NewInstance();
      colors->DeepCopy(scalars);
      }
    vtkMultiplyColorsWithAlpha(colors);
    }

  vtkUnsignedCharArray* colorArray = vtkUnsignedCharArray::SafeDownCast(colors);
  if (!colorArray)
    {
    vtkErrorMacro("Error: color array not of type unsigned char...");
    return;
    }
  unsigned char* ptr = colorArray->GetPointer(0);
  vtkIdType numValues =
      colorArray->GetNumberOfTuples() * colorArray->GetNumberOfComponents();
  this->Internal->colorComponents = colorArray->GetNumberOfComponents();
  this->Internal->colors.reserve(numValues);
  this->Internal->colors.assign(ptr, ptr + numValues);

  colors->Delete();
}

//-----------------------------------------------------------------------------
void vtkVBOPolyDataMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
