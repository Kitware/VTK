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

#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkOpenGL2Texture.h"

// Bring in our fragment lit shader symbols.
#include "vtkglPolyDataVSFragmentLit.h"
#include "vtkglPolyDataFSHeadlight.h"
#include "vtkglPolyDataFSLightKit.h"
#include "vtkglPolyDataFSPositionalLights.h"

// bring in vertex lit shader symbols
//#include "vtkglPolyDataVSLightKit.h"
//#include "vtkglPolyDataVSHeadlight.h"
//#include "vtkglPolyDataVSPositionalLights.h"
#include "vtkglPolyDataVSNoLighting.h"
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
  vtkgl::CellBO *lastBoundBO;

  vtkTimeStamp propertiesTime;

  Private()
  {
  }

};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkVBOPolyDataMapper)

//-----------------------------------------------------------------------------
vtkVBOPolyDataMapper::vtkVBOPolyDataMapper()
  : Internal(new Private), UsingScalarColoring(false)
{
  this->InternalColorTexture = 0;
}


//-----------------------------------------------------------------------------
vtkVBOPolyDataMapper::~vtkVBOPolyDataMapper()
{
  delete this->Internal;
  if (this->InternalColorTexture)
    { // Resources released previously.
    this->InternalColorTexture->Delete();
    this->InternalColorTexture = 0;
    }
}

//-----------------------------------------------------------------------------
void vtkVBOPolyDataMapper::ReleaseGraphicsResources(vtkWindow* win)
{
  // FIXME: Implement resource release.
    // We may not want to do this here.
  if (this->InternalColorTexture)
    {
    this->InternalColorTexture->ReleaseGraphicsResources(win);
    }
}

//-----------------------------------------------------------------------------
void vtkVBOPolyDataMapper::UpdateShader(vtkgl::CellBO &cellBO, vtkRenderer* ren, vtkActor *actor)
{
  int lightComplexity = 0;

  // wacky backwards compatibility with old VTK lighting
  // soooo there are many factors that determine if a primative is lit or not.
  // three that mix in a complex way are representation POINT, Interpolation FLAT
  // and having normals or not.
  bool needLighting = false;
  bool haveNormals = (this->GetInput()->GetPointData()->GetNormals() != NULL);
  if (actor->GetProperty()->GetRepresentation() == VTK_POINTS)
    {
    needLighting = (actor->GetProperty()->GetInterpolation() != VTK_FLAT && haveNormals);
    }
  else  // wireframe or surface rep
    {
    bool isTrisOrStrips = (&cellBO == &this->Internal->tris || &cellBO == &this->Internal->triStrips);
    needLighting = (isTrisOrStrips ||
      (!isTrisOrStrips && actor->GetProperty()->GetInterpolation() != VTK_FLAT && haveNormals));
    }

  // do we need lighting?
  if (actor->GetProperty()->GetLighting() && needLighting)
    {
    // consider the lighting complexity to determine which case applies
    // simple headlight, Light Kit, the whole feature set of VTK
    lightComplexity = 1;
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
    }

  // pick which shader code to use based on above factors
  switch (lightComplexity)
    {
    case 0:
        cellBO.vsFile = vtkglPolyDataVSNoLighting;
        cellBO.fsFile = vtkglPolyDataFS;
      break;
    case 1:
        cellBO.vsFile = vtkglPolyDataVSFragmentLit;
        cellBO.fsFile = vtkglPolyDataFSHeadlight;
      break;
    case 2:
        cellBO.vsFile = vtkglPolyDataVSFragmentLit;
        cellBO.fsFile = vtkglPolyDataFSLightKit;
      break;
    case 3:
        cellBO.vsFile = vtkglPolyDataVSFragmentLit;
        cellBO.fsFile = vtkglPolyDataFSPositionalLights;
      break;
    }
    //cellBO.vsFile = vtkglPolyDataVSHeadlight;
    //cellBO.fsFile = vtkglPolyDataFS;

  if (this->Internal->lastBoundBO &&
      this->Internal->lastBoundBO->vsFile == cellBO.vsFile &&
      this->Internal->lastBoundBO->fsFile == cellBO.fsFile)
    {
      return;
    }

  // Build our shader if necessary.
  std::string VSSource = cellBO.vsFile;
  std::string FSSource = cellBO.fsFile;
  if (this->Internal->layout.ColorComponents != 0)
    {
    VSSource = replace(VSSource,
                                 "//VTK::Color::Dec",
                                 "attribute vec4 diffuseColor;");
    }
  else
    {
    VSSource = replace(VSSource,
                                 "//VTK::Color::Dec",
                                 "uniform vec4 diffuseColor;");
    }
  // normals?
  if (this->Internal->layout.NormalOffset)
    {
    VSSource = replace(VSSource,
                                 "//VTK::Normal::Dec",
                                 "attribute vec3 normalMC; varying vec3 normalVC;");
    VSSource = replace(VSSource,
                                 "//VTK::Normal::Impl",
                                 "normalVC = normalMatrix * normalMC;");
    FSSource = replace(FSSource,
                                 "//VTK::Normal::Dec",
                                 "varying vec3 normalVC;");
    FSSource = replace(FSSource,
                                 "//VTK::Normal::Impl","  if (!gl_FrontFacing) normalVC = -normalVC;");
    }
  else
    {
    VSSource = replace(VSSource,"//VTK::Normal::Dec","");
    VSSource = replace(VSSource,"//VTK::Normal::Impl","");
    FSSource = replace(FSSource,"//VTK::Normal::Dec","");
    if (actor->GetProperty()->GetRepresentation() == VTK_WIREFRAME)
      {
      // generate a normal for lines, it will be perpendicular to the line
      // and maximally aligned with the camera view direction
      // no clue if this is the best way to do this.
      FSSource = replace(FSSource,"//VTK::Normal::Impl",
                                   "vec3 normalVC; if (abs(dot(dFdx(vertexVC.xyz),vec3(1,1,1))) > abs(dot(dFdy(vertexVC.xyz),vec3(1,1,1)))) { normalVC = normalize(cross(cross(dFdx(vertexVC.xyz), vec3(0,0,1)), dFdx(vertexVC.xyz))); } else { normalVC = normalize(cross(cross(dFdy(vertexVC.xyz), vec3(0,0,1)), dFdy(vertexVC.xyz)));}");
      }
    else
      {
      FSSource = replace(FSSource,"//VTK::Normal::Impl",
                                   "vec3 normalVC = normalize(cross(dFdx(vertexVC.xyz), dFdy(vertexVC.xyz)));");
      }
    }
  if (this->Internal->layout.TCoordComponents)
    {
    if (this->Internal->layout.TCoordComponents == 1)
      {
      VSSource = vtkgl::replace(VSSource,
                                   "//VTK::TCoord::Dec",
                                   "attribute float tcoordMC; varying float tcoordVC;");
      VSSource = vtkgl::replace(VSSource,
                                   "//VTK::TCoord::Impl",
                                   "tcoordVC = tcoordMC;");
      FSSource = vtkgl::replace(FSSource,
                                   "//VTK::TCoord::Dec",
                                   "varying float tcoordVC; uniform sampler2D texture1;");
      FSSource = vtkgl::replace(FSSource,
                                   "//VTK::TCoord::Impl",
                                   "gl_FragColor = gl_FragColor*texture2D(texture1, vec2(tcoordVC,0));");
      }
    else
      {
      VSSource = vtkgl::replace(VSSource,
                                   "//VTK::TCoord::Dec",
                                   "attribute vec2 tcoordMC; varying vec2 tcoordVC;");
      VSSource = vtkgl::replace(VSSource,
                                   "//VTK::TCoord::Impl",
                                   "tcoordVC = tcoordMC;");
      FSSource = vtkgl::replace(FSSource,
                                   "//VTK::TCoord::Dec",
                                   "varying vec2 tcoordVC; uniform sampler2D texture1;");
      FSSource = vtkgl::replace(FSSource,
                                   "//VTK::TCoord::Impl",
                                   "gl_FragColor = gl_FragColor*texture2D(texture1, tcoordVC.st);");
      }
    }
  else
    {
    VSSource = vtkgl::replace(VSSource,
                                 "//VTK::TCoord::Dec","");
    VSSource = vtkgl::replace(VSSource,
                                 "//VTK::TCoord::Impl","");
    FSSource = vtkgl::replace(FSSource,
                                 "//VTK::TCoord::Dec","");
    FSSource = vtkgl::replace(FSSource,
                                 "//VTK::TCoord::Impl","");
    }

  //cout << "VS: " << VSSource << endl;
  //cout << "FS: " << FSSource << endl;

  // only recompile if the shader has changed
  if (cellBO.vs.GetSource().size() == 0 ||
      cellBO.vs.GetSource().compare(VSSource) ||
      cellBO.fs.GetSource().compare(FSSource))
    {
    cellBO.vs.SetSource(VSSource);
    cellBO.vs.SetType(vtkgl::Shader::Vertex);
    cellBO.fs.SetSource(FSSource);
    cellBO.fs.SetType(vtkgl::Shader::Fragment);

    if (!cellBO.vs.Compile())
      {
      vtkErrorMacro(<< cellBO.vs.GetError());
      }
    if (!cellBO.fs.Compile())
      {
      vtkErrorMacro(<< cellBO.fs.GetError());
      }
    if (!cellBO.program.AttachShader(cellBO.vs))
      {
      vtkErrorMacro(<< cellBO.program.GetError());
      }
    if (!cellBO.program.AttachShader(cellBO.fs))
      {
      vtkErrorMacro(<< cellBO.program.GetError());
      }
    if (!cellBO.program.Link())
      {
      vtkErrorMacro(<< "Links failed: " << cellBO.program.GetError());
      }
    cellBO.buildTime.Modified();
    }

  // Now to update the VAO too, if necessary.
  vtkgl::VBOLayout &layout = this->Internal->layout;
  if (cellBO.indexCount && this->VBOUpdateTime > cellBO.attributeUpdateTime)
    {
    cellBO.program.Bind();
    cellBO.vao.Bind();
    if (!cellBO.vao.AddAttributeArray(cellBO.program, this->Internal->vbo,
                                    "vertexMC", layout.VertexOffset,
                                    layout.Stride, VTK_FLOAT, 3, false))
      {
      vtkErrorMacro(<< "Error setting 'vertexMC' in triangle VAO.");
      }
    if (layout.NormalOffset)
      {
      if (!cellBO.vao.AddAttributeArray(cellBO.program, this->Internal->vbo,
                                      "normalMC", layout.NormalOffset,
                                      layout.Stride, VTK_FLOAT, 3, false))
        {
        vtkErrorMacro(<< "Error setting 'normalMC' in triangle VAO.");
        }
      }
    if (layout.TCoordComponents)
      {
      if (!cellBO.vao.AddAttributeArray(cellBO.program, this->Internal->vbo,
                                      "tcoordMC", layout.TCoordOffset,
                                      layout.Stride, VTK_FLOAT, layout.TCoordComponents, false))
        {
        vtkErrorMacro(<< "Error setting 'tcoordMC' in shader VAO.");
        }
      }
    if (layout.ColorComponents != 0)
      {
      if (!cellBO.vao.AddAttributeArray(cellBO.program, this->Internal->vbo,
                                      "diffuseColor", layout.ColorOffset,
                                      layout.Stride, VTK_UNSIGNED_CHAR,
                                      layout.ColorComponents, true))
        {
        vtkErrorMacro(<< "Error setting 'diffuseColor' in triangle VAO.");
        }
      }
    cellBO.attributeUpdateTime.Modified();
    }


  if (!cellBO.program.Bind())
    {
    vtkErrorMacro(<< cellBO.program.GetError());
    return;
    }

  if (layout.TCoordComponents)
    {
    cellBO.program.SetUniformValue("texture1", 0);
    }

  this->SetPropertyShaderParameters(cellBO, ren, actor);
  this->SetCameraShaderParameters(cellBO, ren, actor);
  this->SetLightingShaderParameters(cellBO, ren, actor);
  cellBO.vao.Bind();

  this->Internal->lastBoundBO = &cellBO;
}

//-----------------------------------------------------------------------------
void vtkVBOPolyDataMapper::SetLightingShaderParameters(vtkgl::CellBO &cellBO,
                                                      vtkRenderer* ren, vtkActor *vtkNotUsed(actor))
{
  // for unlit and headlight there are no lighting parameters
  if (cellBO.fsFile == vtkglPolyDataFSHeadlight ||
      cellBO.vsFile == vtkglPolyDataVSNoLighting)
    {
    return;
    }

  vtkgl::ShaderProgram &program = cellBO.program;

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

  program.SetUniformValue("lightColor", numberOfLights, lightColor);
  program.SetUniformValue("lightDirectionVC", numberOfLights, lightDirection);
  program.SetUniformValue("numberOfLights", numberOfLights);

  if (cellBO.fsFile == vtkglPolyDataFSLightKit)
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
void vtkVBOPolyDataMapper::SetCameraShaderParameters(vtkgl::CellBO &cellBO,
                                                    vtkRenderer* ren, vtkActor *actor)
{
  vtkgl::ShaderProgram &program = cellBO.program;

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

  tmpMat->DeepCopy(cam->GetProjectionTransformMatrix(ren));
  program.SetUniformValue("VCDCMatrix", tmpMat);

  // for lit shaders set normal matrix
  if (cellBO.vsFile != vtkglPolyDataVSNoLighting)
    {
    // set the normal matrix and send it down
    // (make this a function in camera at some point returning a 3x3)
    tmpMat->DeepCopy(cam->GetViewTransformMatrix());
    if (!actor->GetIsIdentity())
      {
      vtkMatrix4x4::Multiply4x4(tmpMat, actor->GetMatrix(), tmpMat);
      vtkTransform *aTF = vtkTransform::New();
      aTF->SetMatrix(tmpMat);
      double *scale = aTF->GetScale();
      aTF->Scale(1.0/scale[0],1.0/scale[1],1.0/scale[2]);
      tmpMat->DeepCopy(aTF->GetMatrix());
      }
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
    tmpMat3d->Delete();
    }

  tmpMat->Delete();
}

//-----------------------------------------------------------------------------
void vtkVBOPolyDataMapper::SetPropertyShaderParameters(vtkgl::CellBO &cellBO,
                                                       vtkRenderer*, vtkActor *actor)
{
  vtkgl::ShaderProgram &program = cellBO.program;

  // Query the actor for some of the properties that can be applied.
  float opacity = static_cast<float>(actor->GetProperty()->GetOpacity());
  double *aColor = actor->GetProperty()->GetAmbientColor();
  double aIntensity = actor->GetProperty()->GetAmbient();  // ignoring renderer ambient
  vtkgl::Vector3ub ambientColor(static_cast<unsigned char>(aColor[0] * aIntensity * 255.0),
                         static_cast<unsigned char>(aColor[1] * aIntensity * 255.0),
                         static_cast<unsigned char>(aColor[2] * aIntensity * 255.0));
  double *dColor = actor->GetProperty()->GetDiffuseColor();
  double dIntensity = actor->GetProperty()->GetDiffuse();
  vtkgl::Vector4ub diffuseColor(static_cast<unsigned char>(dColor[0] * dIntensity * 255.0),
                         static_cast<unsigned char>(dColor[1] * dIntensity * 255.0),
                         static_cast<unsigned char>(dColor[2] * dIntensity * 255.0),
                         static_cast<unsigned char>(opacity*255.0));
  double *sColor = actor->GetProperty()->GetSpecularColor();
  double sIntensity = actor->GetProperty()->GetSpecular();
  vtkgl::Vector3ub specularColor(static_cast<unsigned char>(sColor[0] * sIntensity * 255.0),
                         static_cast<unsigned char>(sColor[1] * sIntensity * 255.0),
                         static_cast<unsigned char>(sColor[2] * sIntensity * 255.0));
  float specularPower = actor->GetProperty()->GetSpecularPower();

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

  // if there are no points then we are done
  if (!this->GetInput()->GetPoints())
    {
    return;
    }

  this->TimeToDraw = 0.0;

  // Update the VBO if needed.
  if (this->VBOUpdateTime < this->GetMTime() ||
      this->VBOUpdateTime < actor->GetMTime() ||
      this->VBOUpdateTime < input->GetMTime() )
    {
    this->UpdateVBO(actor);
    this->VBOUpdateTime.Modified();
    }


  // If we are coloring by texture, then load the texture map.
  // Use Map as indicator, because texture hangs around.
  if (this->InternalColorTexture)
    {
    this->InternalColorTexture->Load(ren);
    }

  // Bind the VBO, this is shared between the different primitive/cell types.
  this->Internal->vbo.Bind();
  vtkgl::VBOLayout &layout = this->Internal->layout;

  this->Internal->lastBoundBO = NULL;

  // Set the PointSize and LineWidget
  glPointSize(actor->GetProperty()->GetPointSize());
  glLineWidth(actor->GetProperty()->GetLineWidth()); // supported by all OpenGL versions

  // draw points
  if (this->Internal->points.indexCount)
    {
    // Update/build/etc the shader.
    this->UpdateShader(this->Internal->points, ren, actor);
    this->Internal->points.ibo.Bind();
    glDrawRangeElements(GL_POINTS, 0,
                        static_cast<GLuint>(layout.VertexCount - 1),
                        static_cast<GLsizei>(this->Internal->points.indexCount),
                        GL_UNSIGNED_INT,
                        reinterpret_cast<const GLvoid *>(NULL));
    this->Internal->points.ibo.Release();
    }

  // draw lines
  if (this->Internal->lines.indexCount)
    {
    this->UpdateShader(this->Internal->lines, ren, actor);
    this->Internal->lines.ibo.Bind();
    if (actor->GetProperty()->GetRepresentation() == VTK_POINTS)
      {
      glDrawRangeElements(GL_POINTS, 0,
                          static_cast<GLuint>(layout.VertexCount - 1),
                          static_cast<GLsizei>(this->Internal->lines.indexCount),
                          GL_UNSIGNED_INT,
                          reinterpret_cast<const GLvoid *>(NULL));
      }
    else
      {
      for (int eCount = 0; eCount < this->Internal->lines.offsetArray.size(); ++eCount)
        {
        glDrawElements(GL_LINE_STRIP,
          this->Internal->lines.elementsArray[eCount],
          GL_UNSIGNED_INT,
          (GLvoid *)(this->Internal->lines.offsetArray[eCount]));
        }
      }
    this->Internal->lines.ibo.Release();
    }

  // draw polygons
  if (this->Internal->tris.indexCount)
    {
    // First we do the triangles, update the shader, set uniforms, etc.
    this->UpdateShader(this->Internal->tris, ren, actor);
    this->Internal->tris.ibo.Bind();
    if (actor->GetProperty()->GetRepresentation() == VTK_POINTS)
      {
      glDrawRangeElements(GL_POINTS, 0,
                          static_cast<GLuint>(layout.VertexCount - 1),
                          static_cast<GLsizei>(this->Internal->tris.indexCount),
                          GL_UNSIGNED_INT,
                          reinterpret_cast<const GLvoid *>(NULL));
      }
    if (actor->GetProperty()->GetRepresentation() == VTK_WIREFRAME)
      {
      // TODO wireframe of triangles is not lit properly right now
      // you either have to generate normals and send them down
      // or use a geometry shader.
      glMultiDrawElements(GL_LINE_LOOP,
                        (GLsizei *)(&this->Internal->tris.elementsArray[0]),
                        GL_UNSIGNED_INT,
                        reinterpret_cast<const GLvoid **>(&(this->Internal->tris.offsetArray[0])),
                        this->Internal->tris.offsetArray.size());
      }
    if (actor->GetProperty()->GetRepresentation() == VTK_SURFACE)
      {
      glDrawRangeElements(GL_TRIANGLES, 0,
                          static_cast<GLuint>(layout.VertexCount - 1),
                          static_cast<GLsizei>(this->Internal->tris.indexCount),
                          GL_UNSIGNED_INT,
                          reinterpret_cast<const GLvoid *>(NULL));
      }
    this->Internal->tris.ibo.Release();
    }

  // draw strips
  if (this->Internal->triStrips.indexCount)
    {
    // Use the tris shader program/VAO, but triStrips ibo.
    this->UpdateShader(this->Internal->triStrips, ren, actor);
    this->Internal->triStrips.ibo.Bind();
    if (actor->GetProperty()->GetRepresentation() == VTK_POINTS)
      {
      glDrawRangeElements(GL_POINTS, 0,
                          static_cast<GLuint>(layout.VertexCount - 1),
                          static_cast<GLsizei>(this->Internal->triStrips.indexCount),
                          GL_UNSIGNED_INT,
                          reinterpret_cast<const GLvoid *>(NULL));
      }
    // TODO fix wireframe
    if (actor->GetProperty()->GetRepresentation() == VTK_WIREFRAME)
      {
      for (int eCount = 0; eCount < this->Internal->triStrips.offsetArray.size(); ++eCount)
        {
        glDrawElements(GL_LINE_STRIP,
          this->Internal->triStrips.elementsArray[eCount],
          GL_UNSIGNED_INT,
          (GLvoid *)(this->Internal->triStrips.offsetArray[eCount]));
        }
      }
    if (actor->GetProperty()->GetRepresentation() == VTK_SURFACE)
      {
      for (int eCount = 0; eCount < this->Internal->triStrips.offsetArray.size(); ++eCount)
        {
        glDrawElements(GL_TRIANGLE_STRIP,
          this->Internal->triStrips.elementsArray[eCount],
          GL_UNSIGNED_INT,
          (GLvoid *)(this->Internal->triStrips.offsetArray[eCount]));
        }
      }
    this->Internal->triStrips.ibo.Release();
    }

  if (this->Internal->lastBoundBO)
    {
    this->Internal->lastBoundBO->vao.Release();
    this->Internal->lastBoundBO->program.Release();
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


  // For vertex coloring, this sets this->Colors as side effect.
  // For texture map coloring, this sets ColorCoordinates
  // and ColorTextureMap as a side effect.
  // I moved this out of the conditional because it is fast.
  // Color arrays are cached. If nothing has changed,
  // then the scalars do not have to be regenerted.
  this->MapScalars(act->GetProperty()->GetOpacity());

  // If we are coloring by texture, then load the texture map.
  if (this->ColorTextureMap)
    {
    if (this->InternalColorTexture == 0)
      {
      this->InternalColorTexture = vtkOpenGL2Texture::New();
      this->InternalColorTexture->RepeatOff();
      }
    this->InternalColorTexture->SetInputData(this->ColorTextureMap);
    }

  bool cellScalars = false;
  if (this->ScalarVisibility)
    {
    // We must figure out how the scalars should be mapped to the polydata.
    if ( (this->ScalarMode == VTK_SCALAR_MODE_USE_CELL_DATA ||
          this->ScalarMode == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA ||
          this->ScalarMode == VTK_SCALAR_MODE_USE_FIELD_DATA ||
          !poly->GetPointData()->GetScalars() )
         && this->ScalarMode != VTK_SCALAR_MODE_USE_POINT_FIELD_DATA
         && this->Colors)
      {
      cellScalars = true;
      }
    }

  // if we have cell scalars then we have to
  // explode the data
  vtkCellArray *prims[4];
  prims[0] =  poly->GetVerts();
  prims[1] =  poly->GetLines();
  prims[2] =  poly->GetPolys();
  prims[3] =  poly->GetStrips();
  std::vector<unsigned int> cellPointMap;
  std::vector<unsigned int> pointCellMap;
  if (cellScalars)
    {
    vtkgl::CreateCellSupportArrays(poly, prims, cellPointMap, pointCellMap);
    }

  // Mark our properties as updated.
  this->Internal->propertiesTime.Modified();

  // do we have texture maps?
  bool haveTextures = (this->ColorTextureMap || act->GetTexture() || act->GetProperty()->GetNumberOfTextures());

  // Set the texture if we are going to use texture
  // for coloring with a point attribute.
  // fixme ... make the existence of the coordinate array the signal.
  vtkDataArray *tcoords = NULL;
  if (haveTextures)
    {
    if (this->InterpolateScalarsBeforeMapping && this->ColorCoordinates)
      {
      tcoords = this->ColorCoordinates;
      }
    else
      {
      tcoords = poly->GetPointData()->GetTCoords();
      }
    }

  // Iterate through all of the different types in the polydata, building VBOs
  // and IBOs as appropriate for each type.
  this->Internal->layout =
    CreateVBO(poly->GetPoints(),
              cellPointMap.size() > 0 ? cellPointMap.size() : poly->GetPoints()->GetNumberOfPoints(),
              (act->GetProperty()->GetInterpolation() != VTK_FLAT) ? poly->GetPointData()->GetNormals() : NULL,
              tcoords,
              this->Colors ? (unsigned char *)this->Colors->GetVoidPointer(0) : NULL,
              this->Colors ? this->Colors->GetNumberOfComponents() : 0,
              this->Internal->vbo,
              cellPointMap.size() > 0 ? &cellPointMap.front() : NULL,
              pointCellMap.size() > 0 ? &pointCellMap.front() : NULL);

  // create the IBOs
  this->Internal->points.indexCount = CreatePointIndexBuffer(prims[0],
                                                        this->Internal->points.ibo);

  if (act->GetProperty()->GetRepresentation() == VTK_POINTS)
    {
    this->Internal->lines.indexCount = CreatePointIndexBuffer(prims[1],
                         this->Internal->lines.ibo);

    this->Internal->tris.indexCount = CreatePointIndexBuffer(prims[2],
                                                this->Internal->tris.ibo);
    this->Internal->triStrips.indexCount = CreatePointIndexBuffer(prims[3],
                         this->Internal->triStrips.ibo);
    }
  else // WIREFRAME OR SURFACE
    {
    this->Internal->lines.indexCount = CreateMultiIndexBuffer(prims[1],
                           this->Internal->lines.ibo,
                           this->Internal->lines.offsetArray,
                           this->Internal->lines.elementsArray);

    if (act->GetProperty()->GetRepresentation() == VTK_WIREFRAME)
      {
      this->Internal->tris.indexCount = CreateMultiIndexBuffer(prims[2],
                                             this->Internal->tris.ibo,
                                             this->Internal->tris.offsetArray,
                                             this->Internal->tris.elementsArray);
      }
   else // SURFACE
      {
      this->Internal->tris.indexCount = CreateTriangleIndexBuffer(prims[2],
                                                this->Internal->tris.ibo,
                                                poly->GetPoints());
      }

    this->Internal->triStrips.indexCount = CreateMultiIndexBuffer(prims[3],
                           this->Internal->triStrips.ibo,
                           this->Internal->triStrips.offsetArray,
                           this->Internal->triStrips.elementsArray);
    }

  // free up new cell arrays
  if (cellScalars)
    {
    for (int primType = 0; primType < 4; primType++)
      {
      prims[primType]->UnRegister(this);
      }
    }
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
void vtkVBOPolyDataMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
