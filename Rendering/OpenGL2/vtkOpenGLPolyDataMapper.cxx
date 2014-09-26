/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLPolyDataMapper.h"

#include "vtkglVBOHelper.h"

#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkFloatArray.h"
#include "vtkHardwareSelector.h"
#include "vtkImageData.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkMath.h"
#include "vtkMatrix3x3.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLTexture.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkShader.h"
#include "vtkShaderProgram.h"
#include "vtkTransform.h"

#include "vtkOpenGLError.h"

// Bring in our fragment lit shader symbols.
#include "vtkglPolyDataVSFragmentLit.h"
#include "vtkglPolyDataFSHeadlight.h"
#include "vtkglPolyDataFSLightKit.h"
#include "vtkglPolyDataFSPositionalLights.h"

// bring in vertex lit shader symbols
#include "vtkglPolyDataVSNoLighting.h"
#include "vtkglPolyDataFSNoLighting.h"




using vtkgl::replace;

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLPolyDataMapper)

//-----------------------------------------------------------------------------
vtkOpenGLPolyDataMapper::vtkOpenGLPolyDataMapper()
  : UsingScalarColoring(false)
{
  this->InternalColorTexture = 0;
  this->PopulateSelectionSettings = 1;
  this->LastLightComplexity = -1;
  this->LastSelectionState = false;
  this->LastDepthPeeling = 0;
}


//-----------------------------------------------------------------------------
vtkOpenGLPolyDataMapper::~vtkOpenGLPolyDataMapper()
{
  if (this->InternalColorTexture)
    { // Resources released previously.
    this->InternalColorTexture->Delete();
    this->InternalColorTexture = 0;
    }
}

//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::ReleaseGraphicsResources(vtkWindow* win)
{
  this->VBO.ReleaseGraphicsResources();
  this->Points.ReleaseGraphicsResources(win);
  this->Lines.ReleaseGraphicsResources(win);
  this->Tris.ReleaseGraphicsResources(win);
  this->TriStrips.ReleaseGraphicsResources(win);

  if (this->InternalColorTexture)
    {
    this->InternalColorTexture->ReleaseGraphicsResources(win);
    }
  this->Modified();
}


//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::BuildShader(std::string &VSSource,
                                          std::string &FSSource,
                                          std::string &GSSource,
                                          int lightComplexity, vtkRenderer* ren, vtkActor *actor)
{
  this->GetShaderTemplate(VSSource,FSSource,GSSource,lightComplexity, ren, actor);
  this->ReplaceShaderValues(VSSource,FSSource,GSSource,lightComplexity, ren, actor);
}

//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::GetShaderTemplate(std::string &VSSource,
                                          std::string &FSSource,
                                          std::string &GSSource,
                                          int lightComplexity, vtkRenderer*, vtkActor *)
{
  switch (lightComplexity)
    {
    case 0:
        VSSource = vtkglPolyDataVSNoLighting;
        FSSource = vtkglPolyDataFSNoLighting;
      break;
    case 1:
        VSSource = vtkglPolyDataVSFragmentLit;
        FSSource = vtkglPolyDataFSHeadlight;
      break;
    case 2:
        VSSource = vtkglPolyDataVSFragmentLit;
        FSSource = vtkglPolyDataFSLightKit;
      break;
    case 3:
        VSSource = vtkglPolyDataVSFragmentLit;
        FSSource = vtkglPolyDataFSPositionalLights;
      break;
    }
  GSSource.clear();
}

void vtkOpenGLPolyDataMapper::ReplaceShaderValues(std::string &VSSource,
                                                  std::string &FSSource,
                                                  std::string &vtkNotUsed(GSSource),
                                                  int vtkNotUsed(lightComplexity), vtkRenderer* ren, vtkActor *actor)
{
  // Note that the color section will always define vec3 ambientColor, vec3 diffuseColor and float opacity
  if (this->Layout.ColorComponents != 0)
    {
    VSSource = replace(VSSource,"//VTK::Color::Dec",
                                "attribute vec4 scalarColor; varying vec4 vertexColor;");
    VSSource = replace(VSSource,"//VTK::Color::Impl",
                                "vertexColor =  scalarColor;");
    FSSource = replace(FSSource,"//VTK::Color::Dec",
                                "varying vec4 vertexColor;");
    if (this->ScalarMaterialMode == VTK_MATERIALMODE_AMBIENT ||
          (this->ScalarMaterialMode == VTK_MATERIALMODE_DEFAULT && actor->GetProperty()->GetAmbient() > actor->GetProperty()->GetDiffuse()))
      {
      FSSource = replace(FSSource,"//VTK::Color::Impl",
                                  "vec3 ambientColor = vertexColor.rgb;\n"
                                  "vec3 diffuseColor = diffuseColorUniform.rgb;\n"
                                  "float opacity = vertexColor.a;");
      }
    else if (this->ScalarMaterialMode == VTK_MATERIALMODE_DIFFUSE ||
          (this->ScalarMaterialMode == VTK_MATERIALMODE_DEFAULT && actor->GetProperty()->GetAmbient() <= actor->GetProperty()->GetDiffuse()))
      {
      FSSource = replace(FSSource,"//VTK::Color::Impl",
                                  "vec3 diffuseColor = vertexColor.rgb;\n"
                                  "vec3 ambientColor = ambientColorUniform;\n"
                                  "float opacity = vertexColor.a;");
      }
    else
      {
      FSSource = replace(FSSource,"//VTK::Color::Impl",
                                  "vec3 diffuseColor = vertexColor.rgb;\n"
                                  "vec3 ambientColor = vertexColor.rgb;\n"
                                  "float opacity = vertexColor.a;");
      }
    }
  else
    {
    // are we doing scalar coloring by texture?
    if (this->InterpolateScalarsBeforeMapping && this->ColorCoordinates)
      {
      if (this->ScalarMaterialMode == VTK_MATERIALMODE_AMBIENT ||
          (this->ScalarMaterialMode == VTK_MATERIALMODE_DEFAULT &&
            actor->GetProperty()->GetAmbient() > actor->GetProperty()->GetDiffuse()))
        {
        FSSource = vtkgl::replace(FSSource,
                              "//VTK::Color::Impl",
                              "vec4 texColor = texture2D(texture1, tcoordVC.st);\n"
                              "vec3 ambientColor = texColor.rgb;\n"
                              "vec3 diffuseColor = diffuseColorUniform;\n"
                              "float opacity = texColor.a;");
        }
      else if (this->ScalarMaterialMode == VTK_MATERIALMODE_DIFFUSE ||
          (this->ScalarMaterialMode == VTK_MATERIALMODE_DEFAULT &&
           actor->GetProperty()->GetAmbient() <= actor->GetProperty()->GetDiffuse()))
        {
        FSSource = vtkgl::replace(FSSource,
                              "//VTK::Color::Impl",
                              "vec4 texColor = texture2D(texture1, tcoordVC.st);\n"
                              "vec3 ambientColor = ambientColorUniform;\n"
                              "vec3 diffuseColor = texColor.rgb;\n"
                              "float opacity = texColor.a;");
        }
      else
        {
        FSSource = vtkgl::replace(FSSource,
                              "//VTK::Color::Impl",
                              "vec4 texColor = texture2D(texture1, tcoordVC.st);\n"
                              "vec3 ambientColor = texColor.rgb;\n"
                              "vec3 diffuseColor = texColor,rgb;\n"
                              "float opacity = texColor.a;");
        }
      }
    else
      {
      FSSource = replace(FSSource,"//VTK::Color::Impl",
                                "vec3 ambientColor = ambientColorUniform;\n"
                                "vec3 diffuseColor = diffuseColorUniform;\n"
                                "float opacity = opacityUniform;");
      }
    }
  // normals?
  if (this->Layout.NormalOffset)
    {
    VSSource = replace(VSSource,
                                 "//VTK::Normal::Dec",
                                 "attribute vec3 normalMC; varying vec3 normalVCVarying;");
    VSSource = replace(VSSource,
                                 "//VTK::Normal::Impl",
                                 "normalVCVarying = normalMatrix * normalMC;");
    FSSource = replace(FSSource,
                                 "//VTK::Normal::Dec",
                                 "varying vec3 normalVCVarying;");
    FSSource = replace(FSSource,
                                 "//VTK::Normal::Impl",
                                 "vec3 normalVC;\n"
                                 //  if (!gl_Frontfacing) does not work in intel hd4000 mac hence
                                 // the odd version below
                                 "if (int(gl_FrontFacing) == 0) { normalVC = -normalVCVarying; }\n"
                                 "else { normalVC = normalVCVarying; }"
                                 //"normalVC = normalVCVarying;"
                                 );
    }
  else
    {
    FSSource = replace(FSSource,
                                 "//VTK::Normal::Dec",
                                 "#ifdef GL_ES\n"
                                 "#extension GL_OES_standard_derivatives : enable\n"
                                 "#endif\n");
    if (actor->GetProperty()->GetRepresentation() == VTK_WIREFRAME)
      {
      // generate a normal for lines, it will be perpendicular to the line
      // and maximally aligned with the camera view direction
      // no clue if this is the best way to do this.
      // the code below has been optimized a bit so what follows is
      // an explanation of the basic approach. Compute the gradient of the line
      // with respect to x and y, the the larger of the two
      // cross that with the camera view direction. That gives a vector
      // orthogonal to the camera view and the line. Note that the line and the camera
      // view are probably not orthogonal. Which is why when we cross result that with
      // the line gradient again we get a reasonable normal. It will be othogonal to
      // the line (which is a plane but maximally aligned with the camera view.
      FSSource = replace(FSSource,"//VTK::Normal::Impl",
                         "vec3 normalVC;\n"
                         "vec3 fdx = normalize(vec3(dFdx(vertexVC.x),dFdx(vertexVC.y),dFdx(vertexVC.z)));\n"
                         "vec3 fdy = normalize(vec3(dFdy(vertexVC.x),dFdy(vertexVC.y),dFdy(vertexVC.z)));\n"
                         "if (abs(fdx.x) > 0.0)\n"
                         " { normalVC = normalize(cross(vec3(fdx.y, -fdx.x, 0.0), fdx)); }\n"
                         "else { normalVC = normalize(cross(vec3(fdy.y, -fdy.x, 0.0), fdy));}"
                         );
      }
    else
      {
      FSSource = replace(FSSource,"//VTK::Normal::Impl",
                         "vec3 fdx = normalize(vec3(dFdx(vertexVC.x),dFdx(vertexVC.y),dFdx(vertexVC.z)));\n"
                         "vec3 fdy = normalize(vec3(dFdy(vertexVC.x),dFdy(vertexVC.y),dFdy(vertexVC.z)));\n"
                         "vec3 normalVC = normalize(cross(fdx,fdy));\n"
                         // the code below is faster, but does not work on some devices
                         //"vec3 normalVC = normalize(cross(dFdx(vertexVC.xyz), dFdy(vertexVC.xyz)));\n"
                         "if (normalVC.z < 0.0) { normalVC = -1.0*normalVC; }"
                         );
      }
    }
  if (this->Layout.TCoordComponents)
    {
    if (this->Layout.TCoordComponents == 1)
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
                                   "gl_FragColor = gl_FragColor*texture2D(texture1, vec2(tcoordVC,0.0));");
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
      // do texture mapping except for scalat coloring case which is handled above
      if (!this->InterpolateScalarsBeforeMapping || !this->ColorCoordinates)
        {
        FSSource = vtkgl::replace(FSSource,
                                     "//VTK::TCoord::Impl",
                                     "gl_FragColor = gl_FragColor*texture2D(texture1, tcoordVC.st);");
        }
      }

    // handle color mapping by texture

    }


  vtkHardwareSelector* selector = ren->GetSelector();
  bool picking = (ren->GetRenderWindow()->GetIsPicking() || selector != NULL);
  if (picking)
    {
    FSSource = vtkgl::replace(FSSource,
                                 "//VTK::Picking::Dec",
                                 "uniform vec3 mapperIndex;\n"
                                 "uniform int pickingAttributeIDOffset;");
    FSSource = vtkgl::replace(FSSource,
                              "//VTK::Picking::Impl",
                              "if (mapperIndex == vec3(0.0,0.0,0.0))\n"
                              "  {\n"
                              "  int idx = gl_PrimitiveID + 1 + pickingAttributeIDOffset;\n"
                              "  gl_FragColor = vec4((idx%256)/255.0, ((idx/256)%256)/255.0, (idx/65536)/255.0, 1.0);\n"
                              "  }\n"
                              "else\n"
                              "  {\n"
                              "  gl_FragColor = vec4(mapperIndex,1.0);\n"
                              "  }");
    }

  if (ren->GetLastRenderingUsedDepthPeeling())
    {
    FSSource = vtkgl::replace(FSSource,
      "//VTK::DepthPeeling::Dec",
      "uniform vec2 screenSize;\n"
      "uniform sampler2D opaqueZTexture;\n"
      "uniform sampler2D translucentZTexture;\n");
    FSSource = vtkgl::replace(FSSource,
      "//VTK::DepthPeeling::Impl",
      "float odepth = texture2D(opaqueZTexture, gl_FragCoord.xy/screenSize).r;\n"
      "if (gl_FragCoord.z >= odepth) { discard; }\n"
      "float tdepth = texture2D(translucentZTexture, gl_FragCoord.xy/screenSize).r;\n"
      "if (gl_FragCoord.z <= tdepth) { discard; }\n"
    //  "gl_FragColor = vec4(odepth*odepth,tdepth*tdepth,gl_FragCoord.z*gl_FragCoord.z,1.0);"
      );
    }

  //cout << "VS: " << VSSource << endl;
  //cout << "FS: " << FSSource << endl;
}

//-----------------------------------------------------------------------------
bool vtkOpenGLPolyDataMapper::GetNeedToRebuildShader(vtkgl::CellBO &cellBO, vtkRenderer* ren, vtkActor *actor)
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
    bool isTrisOrStrips = (&cellBO == &this->Tris || &cellBO == &this->TriStrips);
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

  if (this->LastLightComplexity != lightComplexity)
    {
    this->LightComplexityChanged.Modified();
    this->LastLightComplexity = lightComplexity;
    }

  if (this->LastDepthPeeling !=
      ren->GetLastRenderingUsedDepthPeeling())
    {
    this->DepthPeelingChanged.Modified();
    this->LastDepthPeeling = ren->GetLastRenderingUsedDepthPeeling();
    }

  vtkHardwareSelector* selector = ren->GetSelector();
  bool picking = (ren->GetIsPicking() || selector != NULL);
  if (this->LastSelectionState != picking)
    {
    this->SelectionStateChanged.Modified();
    this->LastSelectionState = picking;
    }

  // has something changed that would require us to recreate the shader?
  // candidates are
  // property modified (representation interpolation and lighting)
  // input modified
  // light complexity changed
  if (cellBO.Program == 0 ||
      cellBO.ShaderSourceTime < this->GetMTime() ||
      cellBO.ShaderSourceTime < actor->GetMTime() ||
      cellBO.ShaderSourceTime < this->GetInput()->GetMTime() ||
      cellBO.ShaderSourceTime < this->SelectionStateChanged ||
      cellBO.ShaderSourceTime < this->DepthPeelingChanged ||
      cellBO.ShaderSourceTime < this->LightComplexityChanged)
    {
    return true;
    }

  return false;
}

//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::UpdateShader(vtkgl::CellBO &cellBO, vtkRenderer* ren, vtkActor *actor)
{
  vtkOpenGLRenderWindow *renWin = vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());

  // has something changed that would require us to recreate the shader?
  if (this->GetNeedToRebuildShader(cellBO, ren, actor))
    {
    // build the shader source code
    std::string VSSource;
    std::string FSSource;
    std::string GSSource;
    this->BuildShader(VSSource,FSSource,GSSource,this->LastLightComplexity,ren,actor);

    // compile and bind it if needed
    vtkShaderProgram *newShader =
      renWin->GetShaderCache()->ReadyShader(VSSource.c_str(),
                                            FSSource.c_str(),
                                            GSSource.c_str());

    // if the shader changed reinitialize the VAO
    if (newShader != cellBO.Program)
      {
      cellBO.Program = newShader;
      cellBO.vao.ShaderProgramChanged(); // reset the VAO as the shader has changed
      }

    cellBO.ShaderSourceTime.Modified();
    }
  else
    {
    renWin->GetShaderCache()->ReadyShader(cellBO.Program);
    }

  vtkOpenGLCheckErrorMacro("failed after Render");

  this->SetMapperShaderParameters(cellBO, ren, actor);
  vtkOpenGLCheckErrorMacro("failed after Render");
  this->SetPropertyShaderParameters(cellBO, ren, actor);
  vtkOpenGLCheckErrorMacro("failed after Render");
  this->SetCameraShaderParameters(cellBO, ren, actor);
  vtkOpenGLCheckErrorMacro("failed after Render");
  this->SetLightingShaderParameters(cellBO, ren, actor);
  vtkOpenGLCheckErrorMacro("failed after Render");
  cellBO.vao.Bind();

  this->LastBoundBO = &cellBO;
}

void vtkOpenGLPolyDataMapper::SetMapperShaderParameters(vtkgl::CellBO &cellBO,
                                                      vtkRenderer* ren, vtkActor *actor)
{
  // Now to update the VAO too, if necessary.
  vtkgl::VBOLayout &layout = this->Layout;

  if (cellBO.indexCount && (this->OpenGLUpdateTime > cellBO.attributeUpdateTime ||
      cellBO.ShaderSourceTime > cellBO.attributeUpdateTime))
    {
    cellBO.vao.Bind();
    if (!cellBO.vao.AddAttributeArray(cellBO.Program, this->VBO,
                                    "vertexMC", layout.VertexOffset,
                                    layout.Stride, VTK_FLOAT, 3, false))
      {
      vtkErrorMacro(<< "Error setting 'vertexMC' in shader VAO.");
      }
    if (layout.NormalOffset && this->LastLightComplexity > 0)
      {
      if (!cellBO.vao.AddAttributeArray(cellBO.Program, this->VBO,
                                      "normalMC", layout.NormalOffset,
                                      layout.Stride, VTK_FLOAT, 3, false))
        {
        vtkErrorMacro(<< "Error setting 'normalMC' in shader VAO.");
        }
      }
    if (layout.TCoordComponents)
      {
      if (!cellBO.vao.AddAttributeArray(cellBO.Program, this->VBO,
                                      "tcoordMC", layout.TCoordOffset,
                                      layout.Stride, VTK_FLOAT, layout.TCoordComponents, false))
        {
        vtkErrorMacro(<< "Error setting 'tcoordMC' in shader VAO.");
        }
      }
    if (layout.ColorComponents != 0)
      {
      if (!cellBO.vao.AddAttributeArray(cellBO.Program, this->VBO,
                                      "scalarColor", layout.ColorOffset,
                                      layout.Stride, VTK_UNSIGNED_CHAR,
                                      layout.ColorComponents, true))
        {
        vtkErrorMacro(<< "Error setting 'scalarColor' in shader VAO.");
        }
      }
    cellBO.attributeUpdateTime.Modified();
    }

  if (layout.TCoordComponents)
    {
    vtkTexture *texture = actor->GetTexture();
    if (this->ColorTextureMap)
      {
      texture = this->InternalColorTexture;
      }
    if (!texture && actor->GetProperty()->GetNumberOfTextures())
      {
      texture = actor->GetProperty()->GetTexture(0);
      }
    int tunit = vtkOpenGLTexture::SafeDownCast(texture)->GetTextureUnit();
    cellBO.Program->SetUniformi("texture1", tunit);
    }

  // if depth peeling set the required uniforms
  if (ren->GetLastRenderingUsedDepthPeeling())
    {
    vtkOpenGLRenderer *oglren = vtkOpenGLRenderer::SafeDownCast(ren);
    int otunit = oglren->GetOpaqueZTextureUnit();
    cellBO.Program->SetUniformi("opaqueZTexture", otunit);

    int ttunit = oglren->GetTranslucentZTextureUnit();
    cellBO.Program->SetUniformi("translucentZTexture", ttunit);

    int *renSize = ren->GetSize();
    float screenSize[2];
    screenSize[0] = renSize[0];
    screenSize[1] = renSize[1];
    cellBO.Program->SetUniform2f("screenSize", screenSize);
    }

  if (this->LastSelectionState)
    {
    cellBO.Program->SetUniformi("pickingAttributeIDOffset", this->pickingAttributeIDOffset);
    vtkHardwareSelector* selector = ren->GetSelector();
    if (selector)
      {
      if (selector->GetCurrentPass() == vtkHardwareSelector::ID_LOW24)
        {
        float tmp[3] = {0,0,0};
        cellBO.Program->SetUniform3f("mapperIndex", tmp);
        }
      else
        {
        cellBO.Program->SetUniform3f("mapperIndex", selector->GetPropColorValue());
        }
      }
    else
      {
      unsigned int idx = ren->GetCurrentPickId();
      float color[3];
      vtkHardwareSelector::Convert(idx, color);
      cellBO.Program->SetUniform3f("mapperIndex", color);
      }
    }
}

//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::SetLightingShaderParameters(vtkgl::CellBO &cellBO,
                                                      vtkRenderer* ren, vtkActor *vtkNotUsed(actor))
{
  // for unlit and headlight there are no lighting parameters
  if (this->LastLightComplexity < 2)
    {
    return;
    }

  vtkShaderProgram *program = cellBO.Program;

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

  program->SetUniform3fv("lightColor", numberOfLights, lightColor);
  program->SetUniform3fv("lightDirectionVC", numberOfLights, lightDirection);
  program->SetUniformi("numberOfLights", numberOfLights);

  // we are done unless we have positional lights
  if (this->LastLightComplexity < 3)
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
      double *tlp = viewTF->TransformPoint(lp);
      lightPosition[numberOfLights][0] = tlp[0];
      lightPosition[numberOfLights][1] = tlp[1];
      lightPosition[numberOfLights][2] = tlp[2];
      lightPositional[numberOfLights] = light->GetPositional();
      numberOfLights++;
      }
    }
  program->SetUniform3fv("lightAttenuation", numberOfLights, lightAttenuation);
  program->SetUniform1iv("lightPositional", numberOfLights, lightPositional);
  program->SetUniform3fv("lightPositionVC", numberOfLights, lightPosition);
  program->SetUniform1fv("lightExponent", numberOfLights, lightExponent);
  program->SetUniform1fv("lightConeAngle", numberOfLights, lightConeAngle);
}

//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::SetCameraShaderParameters(vtkgl::CellBO &cellBO,
                                                    vtkRenderer* ren, vtkActor *actor)
{
  vtkShaderProgram *program = cellBO.Program;

  vtkCamera *cam = ren->GetActiveCamera();

  vtkNew<vtkMatrix4x4> tmpMat;
  tmpMat->DeepCopy(cam->GetModelViewTransformMatrix());

  // compute the combined ModelView matrix and send it down to save time in the shader
  vtkMatrix4x4::Multiply4x4(tmpMat.Get(), actor->GetMatrix(), tmpMat.Get());

  tmpMat->Transpose();
  program->SetUniformMatrix("MCVCMatrix", tmpMat.Get());

  // for lit shaders set normal matrix
  if (this->LastLightComplexity > 0)
    {
    tmpMat->Transpose();

    // set the normal matrix and send it down
    // (make this a function in camera at some point returning a 3x3)
    // Reuse the matrix we already got (and possibly multiplied with model mat.
    if (!actor->GetIsIdentity())
      {
      vtkNew<vtkTransform> aTF;
      aTF->SetMatrix(tmpMat.Get());
      double *scale = aTF->GetScale();
      aTF->Scale(1.0 / scale[0], 1.0 / scale[1], 1.0 / scale[2]);
      tmpMat->DeepCopy(aTF->GetMatrix());
      }
    vtkNew<vtkMatrix3x3> tmpMat3d;
    for(int i = 0; i < 3; ++i)
      {
      for (int j = 0; j < 3; ++j)
        {
        tmpMat3d->SetElement(i, j, tmpMat->GetElement(i, j));
        }
      }
    tmpMat3d->Invert();
    program->SetUniformMatrix("normalMatrix", tmpMat3d.Get());
    }

  vtkMatrix4x4 *tmpProj;
  tmpProj = cam->GetProjectionTransformMatrix(ren); // allocates a matrix
  program->SetUniformMatrix("VCDCMatrix", tmpProj);
  tmpProj->UnRegister(this);
}

//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::SetPropertyShaderParameters(vtkgl::CellBO &cellBO,
                                                       vtkRenderer*, vtkActor *actor)
{
  vtkShaderProgram *program = cellBO.Program;

  // Query the actor for some of the properties that can be applied.
  float opacity = static_cast<float>(actor->GetProperty()->GetOpacity());
  double *aColor = actor->GetProperty()->GetAmbientColor();
  double aIntensity = actor->GetProperty()->GetAmbient();  // ignoring renderer ambient
  float ambientColor[3] = {static_cast<float>(aColor[0] * aIntensity), static_cast<float>(aColor[1] * aIntensity), static_cast<float>(aColor[2] * aIntensity)};
  double *dColor = actor->GetProperty()->GetDiffuseColor();
  double dIntensity = actor->GetProperty()->GetDiffuse();
  float diffuseColor[3] = {static_cast<float>(dColor[0] * dIntensity), static_cast<float>(dColor[1] * dIntensity), static_cast<float>(dColor[2] * dIntensity)};
  double *sColor = actor->GetProperty()->GetSpecularColor();
  double sIntensity = actor->GetProperty()->GetSpecular();
  float specularColor[3] = {static_cast<float>(sColor[0] * sIntensity), static_cast<float>(sColor[1] * sIntensity), static_cast<float>(sColor[2] * sIntensity)};
  double specularPower = actor->GetProperty()->GetSpecularPower();

  program->SetUniformf("opacityUniform", opacity);
  program->SetUniform3f("ambientColorUniform", ambientColor);
  program->SetUniform3f("diffuseColorUniform", diffuseColor);
  // we are done unless we have lighting
  if (this->LastLightComplexity < 1)
    {
    return;
    }
  program->SetUniform3f("specularColor", specularColor);
  program->SetUniformf("specularPower", specularPower);
}

//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::RenderPieceStart(vtkRenderer* ren, vtkActor *actor)
{
  vtkDataObject *input= this->GetInputDataObject(0, 0);

  vtkHardwareSelector* selector = ren->GetSelector();
  if (selector && this->PopulateSelectionSettings)
    {
    selector->BeginRenderProp();
    if (selector->GetCurrentPass() == vtkHardwareSelector::COMPOSITE_INDEX_PASS)
      {
      selector->RenderCompositeIndex(1);
      }
    if (selector->GetCurrentPass() == vtkHardwareSelector::ID_LOW24 ||
        selector->GetCurrentPass() == vtkHardwareSelector::ID_MID24 ||
        selector->GetCurrentPass() == vtkHardwareSelector::ID_HIGH16)
      {
      selector->RenderAttributeId(0);
      }
    }

  this->TimeToDraw = 0.0;
  this->pickingAttributeIDOffset = 0;

  // Update the OpenGL if needed.
  if (this->OpenGLUpdateTime < this->GetMTime() ||
      this->OpenGLUpdateTime < actor->GetMTime() ||
      this->OpenGLUpdateTime < input->GetMTime() )
    {
    this->UpdateVBO(actor);
    this->OpenGLUpdateTime.Modified();
    }


  // If we are coloring by texture, then load the texture map.
  // Use Map as indicator, because texture hangs around.
  if (this->InternalColorTexture)
    {
    this->InternalColorTexture->Load(ren);
    }

  // Bind the OpenGL, this is shared between the different primitive/cell types.
  this->VBO.Bind();

  this->LastBoundBO = NULL;

  // Set the PointSize and LineWidget
#if GL_ES_VERSION_2_0 != 1
  glPointSize(actor->GetProperty()->GetPointSize()); // not on ES2
#endif
  glLineWidth(actor->GetProperty()->GetLineWidth()); // supported by all OpenGL versions

  if ( this->GetResolveCoincidentTopology() )
    {
    glEnable(GL_POLYGON_OFFSET_FILL);
    if ( this->GetResolveCoincidentTopology() == VTK_RESOLVE_SHIFT_ZBUFFER )
      {
      vtkErrorMacro(<< "GetResolveCoincidentTopologyZShift is not supported use Polygon offset instead");
      // do something rough as better than nothing
      double zRes = this->GetResolveCoincidentTopologyZShift(); // 0 is no shift 1 is big shift
      double f = zRes*4.0;
      glPolygonOffset(f,0.0);  // supported on ES2/3/etc
      }
    else
      {
      double f, u;
      this->GetResolveCoincidentTopologyPolygonOffsetParameters(f,u);
      glPolygonOffset(f,u);  // supported on ES2/3/etc
      }
    }
}

//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::RenderPieceDraw(vtkRenderer* ren, vtkActor *actor)
{
  vtkgl::VBOLayout &layout = this->Layout;

  // draw points
  if (this->Points.indexCount)
    {
    // Update/build/etc the shader.
    this->UpdateShader(this->Points, ren, actor);
    this->Points.ibo.Bind();
    glDrawRangeElements(GL_POINTS, 0,
                        static_cast<GLuint>(layout.VertexCount - 1),
                        static_cast<GLsizei>(this->Points.indexCount),
                        GL_UNSIGNED_INT,
                        reinterpret_cast<const GLvoid *>(NULL));
    this->Points.ibo.Release();
    this->pickingAttributeIDOffset += (int)this->Points.indexCount;
    }

  // draw lines
  if (this->Lines.indexCount)
    {
    this->UpdateShader(this->Lines, ren, actor);
    this->Lines.ibo.Bind();
    if (actor->GetProperty()->GetRepresentation() == VTK_POINTS)
      {
      glDrawRangeElements(GL_POINTS, 0,
                          static_cast<GLuint>(layout.VertexCount - 1),
                          static_cast<GLsizei>(this->Lines.indexCount),
                          GL_UNSIGNED_INT,
                          reinterpret_cast<const GLvoid *>(NULL));
      }
    else
      {
      glMultiDrawElements(GL_LINE_STRIP,
                        (GLsizei *)(&this->Lines.elementsArray[0]),
                        GL_UNSIGNED_INT,
                        reinterpret_cast<const GLvoid **>(&(this->Lines.offsetArray[0])),
                        (GLsizei)this->Lines.offsetArray.size());
      }
    this->Lines.ibo.Release();
    this->pickingAttributeIDOffset += (int)this->Lines.indexCount;
    }

  // draw polygons
  if (this->Tris.indexCount)
    {
    // First we do the triangles, update the shader, set uniforms, etc.
  vtkOpenGLCheckErrorMacro("failed after Render");
    this->UpdateShader(this->Tris, ren, actor);
  vtkOpenGLCheckErrorMacro("failed after Render");
    this->Tris.ibo.Bind();
  vtkOpenGLCheckErrorMacro("failed after Render");
    if (actor->GetProperty()->GetRepresentation() == VTK_POINTS)
      {
      glDrawRangeElements(GL_POINTS, 0,
                          static_cast<GLuint>(layout.VertexCount - 1),
                          static_cast<GLsizei>(this->Tris.indexCount),
                          GL_UNSIGNED_INT,
                          reinterpret_cast<const GLvoid *>(NULL));
      }
    if (actor->GetProperty()->GetRepresentation() == VTK_WIREFRAME)
      {
      glMultiDrawElements(GL_LINE_LOOP,
                        (GLsizei *)(&this->Tris.elementsArray[0]),
                        GL_UNSIGNED_INT,
                        reinterpret_cast<const GLvoid **>(&(this->Tris.offsetArray[0])),
                        (GLsizei)this->Tris.offsetArray.size());
      }
    if (actor->GetProperty()->GetRepresentation() == VTK_SURFACE)
      {
      glDrawRangeElements(GL_TRIANGLES, 0,
                          static_cast<GLuint>(layout.VertexCount - 1),
                          static_cast<GLsizei>(this->Tris.indexCount),
                          GL_UNSIGNED_INT,
                          reinterpret_cast<const GLvoid *>(NULL));
      }
  vtkOpenGLCheckErrorMacro("failed after Render");
    this->Tris.ibo.Release();
  vtkOpenGLCheckErrorMacro("failed after Render");
    this->pickingAttributeIDOffset += (int)this->Tris.indexCount;
    }

  // draw strips
  if (this->TriStrips.indexCount)
    {
    // Use the tris shader program/VAO, but triStrips ibo.
    this->UpdateShader(this->TriStrips, ren, actor);
    this->TriStrips.ibo.Bind();
    if (actor->GetProperty()->GetRepresentation() == VTK_POINTS)
      {
      glDrawRangeElements(GL_POINTS, 0,
                          static_cast<GLuint>(layout.VertexCount - 1),
                          static_cast<GLsizei>(this->TriStrips.indexCount),
                          GL_UNSIGNED_INT,
                          reinterpret_cast<const GLvoid *>(NULL));
      }
    if (actor->GetProperty()->GetRepresentation() == VTK_WIREFRAME)
      {
      glMultiDrawElements(GL_LINE_STRIP,
                        (GLsizei *)(&this->TriStrips.elementsArray[0]),
                        GL_UNSIGNED_INT,
                        reinterpret_cast<const GLvoid **>(&(this->TriStrips.offsetArray[0])),
                        (GLsizei)this->TriStrips.offsetArray.size());
      }
    if (actor->GetProperty()->GetRepresentation() == VTK_SURFACE)
      {
      glMultiDrawElements(GL_TRIANGLE_STRIP,
                        (GLsizei *)(&this->TriStrips.elementsArray[0]),
                        GL_UNSIGNED_INT,
                        reinterpret_cast<const GLvoid **>(&(this->TriStrips.offsetArray[0])),
                        (GLsizei)this->TriStrips.offsetArray.size());
      }
    this->TriStrips.ibo.Release();
    }

}

//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::RenderPieceFinish(vtkRenderer* ren, vtkActor *vtkNotUsed(actor))
{
  vtkHardwareSelector* selector = ren->GetSelector();
  if (selector && this->PopulateSelectionSettings)
    {
    selector->EndRenderProp();
    }

  if (this->LastBoundBO)
    {
    this->LastBoundBO->vao.Release();
    }

  this->VBO.Release();

  if ( this->GetResolveCoincidentTopology() )
    {
    glDisable(GL_POLYGON_OFFSET_FILL);
    }

  if (this->InternalColorTexture)
    {
    this->InternalColorTexture->PostRender(ren);
    }

  // If the timer is not accurate enough, set it to a small
  // time so that it is not zero
  if (this->TimeToDraw == 0.0)
    {
    this->TimeToDraw = 0.0001;
    }

  this->UpdateProgress(1.0);
}

//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::RenderPiece(vtkRenderer* ren, vtkActor *actor)
{
  // Make sure that we have been properly initialized.
  if (ren->GetRenderWindow()->CheckAbortStatus())
    {
    return;
    }

  vtkDataObject *input= this->GetInputDataObject(0, 0);

  if (input == NULL)
    {
    vtkErrorMacro(<< "No input!");
    return;
    }

  this->InvokeEvent(vtkCommand::StartEvent,NULL);
  if (!this->Static)
    {
    this->GetInputAlgorithm()->Update();
    }
  this->InvokeEvent(vtkCommand::EndEvent,NULL);

  // if there are no points then we are done
  if (!this->GetInput()->GetPoints())
    {
    return;
    }

  vtkOpenGLCheckErrorMacro("failed after Render");
  this->RenderPieceStart(ren, actor);
  vtkOpenGLCheckErrorMacro("failed after Render");
  this->RenderPieceDraw(ren, actor);
  vtkOpenGLCheckErrorMacro("failed after Render");
  this->RenderPieceFinish(ren, actor);
  vtkOpenGLCheckErrorMacro("failed after Render");

  // if EdgeVisibility is on then draw the wireframe also
  this->RenderEdges(ren,actor);
}

void vtkOpenGLPolyDataMapper::RenderEdges(vtkRenderer* ren, vtkActor *actor)
{
  vtkProperty *prop = actor->GetProperty();
  bool draw_surface_with_edges =
    (prop->GetEdgeVisibility() && prop->GetRepresentation() == VTK_SURFACE);
  if (draw_surface_with_edges)
    {
    // store old values
    double f, u;
    this->GetResolveCoincidentTopologyPolygonOffsetParameters(f,u);
    double zRes = this->GetResolveCoincidentTopologyZShift();
    int oldRCT = this->GetResolveCoincidentTopology();
    vtkProperty *oldProp = vtkProperty::New();
    oldProp->DeepCopy(prop);

    // setup new values and render
    if (oldRCT == VTK_RESOLVE_SHIFT_ZBUFFER)
      {
      this->SetResolveCoincidentTopologyZShift(zRes*2.0);
      }
    else
      {
      this->SetResolveCoincidentTopology(VTK_RESOLVE_POLYGON_OFFSET);
      this->SetResolveCoincidentTopologyPolygonOffsetParameters(f+0.5,u*1.5);
      }
    prop->LightingOff();
    prop->SetAmbientColor(prop->GetEdgeColor());
    prop->SetAmbient(1.0);
    prop->SetDiffuse(0.0);
    prop->SetSpecular(0.0);
    prop->SetRepresentationToWireframe();
    this->RenderPieceStart(ren, actor);
    this->RenderPieceDraw(ren, actor);
    this->RenderPieceFinish(ren, actor);

    // restore old values
    prop->SetRepresentationToSurface();
    prop->SetLighting(oldProp->GetLighting());
    prop->SetAmbientColor(oldProp->GetAmbientColor());
    prop->SetAmbient(oldProp->GetAmbient());
    prop->SetDiffuse(oldProp->GetDiffuse());
    prop->SetSpecular(oldProp->GetSpecular());
    this->SetResolveCoincidentTopologyPolygonOffsetParameters(f,u);
    this->SetResolveCoincidentTopologyZShift(zRes);
    this->SetResolveCoincidentTopology(oldRCT);
    oldProp->UnRegister(this);

/*
    // Disable textures when rendering the surface edges.
    // This ensures that edges are always drawn solid.
    glDisable(GL_TEXTURE_2D);

    this->Information->Set(vtkPolyDataPainter::DISABLE_SCALAR_COLOR(), 1);
    this->Information->Remove(vtkPolyDataPainter::DISABLE_SCALAR_COLOR());
    */
   }
}


//-------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::ComputeBounds()
{
  if (!this->GetInput())
    {
    vtkMath::UninitializeBounds(this->Bounds);
    return;
    }
  this->GetInput()->GetBounds(this->Bounds);
}

//-------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::UpdateVBO(vtkActor *act)
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
      this->InternalColorTexture = vtkOpenGLTexture::New();
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

  bool cellNormals = false;
  // Do we have cell normals?
  vtkDataArray *n =
    (act->GetProperty()->GetInterpolation() != VTK_FLAT) ? poly->GetPointData()->GetNormals() : NULL;
  if (n == NULL && poly->GetCellData()->GetNormals())
    {
    cellNormals = true;
    n = poly->GetCellData()->GetNormals();
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
  if (cellScalars || cellNormals)
    {
    vtkgl::CreateCellSupportArrays(poly, prims, cellPointMap, pointCellMap);
    }

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

  // Iterate through all of the different types in the polydata, building OpenGLs
  // and IBOs as appropriate for each type.
  this->Layout =
    CreateVBO(poly->GetPoints(),
              cellPointMap.size() > 0 ? (unsigned int)cellPointMap.size() : poly->GetPoints()->GetNumberOfPoints(),
              n, tcoords,
              this->Colors ? (unsigned char *)this->Colors->GetVoidPointer(0) : NULL,
              this->Colors ? this->Colors->GetNumberOfComponents() : 0,
              this->VBO,
              cellPointMap.size() > 0 ? &cellPointMap.front() : NULL,
              pointCellMap.size() > 0 ? &pointCellMap.front() : NULL,
              cellScalars, cellNormals);

  // create the IBOs
  this->Points.indexCount = CreatePointIndexBuffer(prims[0],
                                                   this->Points.ibo);

  if (act->GetProperty()->GetRepresentation() == VTK_POINTS)
    {
    this->Lines.indexCount = CreatePointIndexBuffer(prims[1],
                         this->Lines.ibo);

    this->Tris.indexCount = CreatePointIndexBuffer(prims[2],
                                                this->Tris.ibo);
    this->TriStrips.indexCount = CreatePointIndexBuffer(prims[3],
                         this->TriStrips.ibo);
    }
  else // WIREFRAME OR SURFACE
    {
    this->Lines.indexCount = CreateMultiIndexBuffer(prims[1],
                           this->Lines.ibo,
                           this->Lines.offsetArray,
                           this->Lines.elementsArray, false);

    if (act->GetProperty()->GetRepresentation() == VTK_WIREFRAME)
      {
      this->Tris.indexCount = CreateMultiIndexBuffer(prims[2],
                                             this->Tris.ibo,
                                             this->Tris.offsetArray,
                                             this->Tris.elementsArray, false);
      this->TriStrips.indexCount = CreateMultiIndexBuffer(prims[3],
                           this->TriStrips.ibo,
                           this->TriStrips.offsetArray,
                           this->TriStrips.elementsArray, true);
      }
   else // SURFACE
      {
      this->Tris.indexCount = CreateTriangleIndexBuffer(prims[2],
                                                this->Tris.ibo,
                                                poly->GetPoints(),
                                                cellPointMap);
      this->TriStrips.indexCount = CreateMultiIndexBuffer(prims[3],
                           this->TriStrips.ibo,
                           this->TriStrips.offsetArray,
                           this->TriStrips.elementsArray, false);
      }
    }

  // free up new cell arrays
  if (cellScalars || cellNormals)
    {
    for (int primType = 0; primType < 4; primType++)
      {
      prims[primType]->UnRegister(this);
      }
    }
}

//-----------------------------------------------------------------------------
bool vtkOpenGLPolyDataMapper::GetIsOpaque()
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


//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
