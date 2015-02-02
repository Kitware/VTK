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
#include "vtkDepthPeelingPass.h"
#include "vtkFloatArray.h"
#include "vtkHardwareSelector.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkMath.h"
#include "vtkMatrix3x3.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLActor.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLTexture.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkScalarsToColors.h"
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

#include <algorithm>

using vtkgl::substitute;

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
  this->CurrentInput = 0;
  this->TempMatrix4 = vtkMatrix4x4::New();
  this->TempMatrix3 = vtkMatrix3x3::New();
  this->DrawingEdges = false;
}


//-----------------------------------------------------------------------------
vtkOpenGLPolyDataMapper::~vtkOpenGLPolyDataMapper()
{
  if (this->InternalColorTexture)
    { // Resources released previously.
    this->InternalColorTexture->Delete();
    this->InternalColorTexture = 0;
    }
  this->TempMatrix3->Delete();
  this->TempMatrix4->Delete();
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

bool vtkOpenGLPolyDataMapper::IsShaderVariableUsed(const char *name)
{
  return std::binary_search(this->ShaderVariablesUsed.begin(),
        this->ShaderVariablesUsed.end(), name);
}

//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::BuildShader(std::string &VSSource,
                                          std::string &FSSource,
                                          std::string &GSSource,
                                          int lightComplexity, vtkRenderer* ren, vtkActor *actor)
{
  this->ShaderVariablesUsed.clear();
  this->GetShaderTemplate(VSSource,FSSource,GSSource,lightComplexity, ren, actor);
  this->ReplaceShaderValues(VSSource,FSSource,GSSource,lightComplexity, ren, actor);
  std::sort(this->ShaderVariablesUsed.begin(),this->ShaderVariablesUsed.end());
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

void vtkOpenGLPolyDataMapper::ReplaceShaderColorMaterialValues(std::string &VSSource,
                                                  std::string &FSSource,
                                                  std::string &vtkNotUsed(GSSource),
                                                  int lightComplexity,
                                                  vtkRenderer* vtkNotUsed(ren), vtkActor *actor)
{
  // crate the material/color property declarations, and VS implementation
  // these are always defined
  std::string colorDec =
    "uniform float opacityUniform; // the fragment opacity\n"
    "uniform vec3 ambientColorUniform; // intensity weighted color\n"
    "uniform vec3 diffuseColorUniform; // intensity weighted color\n";
  // add some if we have a backface property
  if (actor->GetBackfaceProperty() && !this->DrawingEdges)
    {
    colorDec +=
      "uniform float opacityUniformBF; // the fragment opacity\n"
      "uniform vec3 ambientColorUniformBF; // intensity weighted color\n"
      "uniform vec3 diffuseColorUniformBF; // intensity weighted color\n";
    }
  // add more for specular
  if (lightComplexity)
    {
    colorDec +=
      "uniform vec3 specularColorUniform; // intensity weighted color\n"
      "uniform float specularPowerUniform;\n";
    if (actor->GetBackfaceProperty())
      {
      colorDec +=
        "uniform vec3 specularColorUniformBF; // intensity weighted color\n"
        "uniform float specularPowerUniformBF;\n";
      }
    }
  // add scalar vertex coloring
  if (this->Layout.ColorComponents != 0)
    {
    colorDec += "varying vec4 vertexColor;\n";
    substitute(VSSource,"//VTK::Color::Dec",
                        "attribute vec4 scalarColor;\n"
                        "varying vec4 vertexColor;");
    substitute(VSSource,"//VTK::Color::Impl",
                        "vertexColor =  scalarColor;");
    }
  substitute(FSSource,"//VTK::Color::Dec", colorDec);

  // now handle the more complex fragment shader implementation
  // the following are always defined variables.  We start
  // by assiging a default value from the uniform
  std::string colorImpl =
    "vec3 ambientColor;\n"
    "  vec3 diffuseColor;\n"
    "  float opacity;\n";
  if (lightComplexity)
    {
    colorImpl +=
      "  vec3 specularColor;\n"
      "  float specularPower;\n";
    }
  if (actor->GetBackfaceProperty())
    {
    if (lightComplexity)
      {
      colorImpl +=
        "  if (int(gl_FrontFacing) == 0) {\n"
        "    ambientColor = ambientColorUniformBF;\n"
        "    diffuseColor = diffuseColorUniformBF;\n"
        "    specularColor = specularColorUniformBF;\n"
        "    specularPower = specularPowerUniformBF;\n"
        "    opacity = opacityUniformBF; }\n"
        "  else {\n"
        "    ambientColor = ambientColorUniform;\n"
        "    diffuseColor = diffuseColorUniform;\n"
        "    specularColor = specularColorUniform;\n"
        "    specularPower = specularPowerUniform;\n"
        "    opacity = opacityUniform; }\n";
      }
    else
      {
      colorImpl +=
        "  if (int(gl_FrontFacing) == 0) {\n"
        "    ambientColor = ambientColorUniformBF;\n"
        "    diffuseColor = diffuseColorUniformBF;\n"
        "    opacity = opacityUniformBF; }\n"
        "  else {\n"
        "    ambientColor = ambientColorUniform;\n"
        "    diffuseColor = diffuseColorUniform;\n"
        "    opacity = opacityUniform; }\n";
      }
    }
  else
    {
    colorImpl +=
      "  ambientColor = ambientColorUniform;\n"
      "  diffuseColor = diffuseColorUniform;\n"
      "  opacity = opacityUniform;\n";
    if (lightComplexity)
      {
      colorImpl +=
        "  specularColor = specularColorUniform;\n"
        "  specularPower = specularPowerUniform;\n";
      }
    }

  // now handle scalar coloring
  if (this->Layout.ColorComponents != 0)
    {
    if (this->ScalarMaterialMode == VTK_MATERIALMODE_AMBIENT ||
          (this->ScalarMaterialMode == VTK_MATERIALMODE_DEFAULT && actor->GetProperty()->GetAmbient() > actor->GetProperty()->GetDiffuse()))
      {
      substitute(FSSource,"//VTK::Color::Impl", colorImpl +
                          "  ambientColor = vertexColor.rgb;\n"
                          "  opacity = opacity*vertexColor.a;");
      }
    else if (this->ScalarMaterialMode == VTK_MATERIALMODE_DIFFUSE ||
          (this->ScalarMaterialMode == VTK_MATERIALMODE_DEFAULT && actor->GetProperty()->GetAmbient() <= actor->GetProperty()->GetDiffuse()))
      {
      substitute(FSSource,"//VTK::Color::Impl", colorImpl +
                          "  diffuseColor = vertexColor.rgb;\n"
                          "  opacity = opacity*vertexColor.a;");
      }
    else
      {
      substitute(FSSource,"//VTK::Color::Impl", colorImpl +
                          "  diffuseColor = vertexColor.rgb;\n"
                          "  ambientColor = vertexColor.rgb;\n"
                          "  opacity = opacity*vertexColor.a;");
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
        substitute(FSSource,
                    "//VTK::Color::Impl", colorImpl +
                    "  vec4 texColor = texture2D(texture1, tcoordVC.st);\n"
                    "  ambientColor = texColor.rgb;\n"
                    "  opacity = opacity*texColor.a;");
        }
      else if (this->ScalarMaterialMode == VTK_MATERIALMODE_DIFFUSE ||
          (this->ScalarMaterialMode == VTK_MATERIALMODE_DEFAULT &&
           actor->GetProperty()->GetAmbient() <= actor->GetProperty()->GetDiffuse()))
        {
        substitute(FSSource,
                    "//VTK::Color::Impl", colorImpl +
                    "  vec4 texColor = texture2D(texture1, tcoordVC.st);\n"
                    "  diffuseColor = texColor.rgb;\n"
                    "  opacity = opacity*texColor.a;");
        }
      else
        {
        substitute(FSSource,
                    "//VTK::Color::Impl", colorImpl +
                    "vec4 texColor = texture2D(texture1, tcoordVC.st);\n"
                    "  ambientColor = texColor.rgb;\n"
                    "  diffuseColor = texColor.rgb;\n"
                    "  opacity = opacity*texColor.a;");
        }
      }
    else
      {
      substitute(FSSource,"//VTK::Color::Impl", colorImpl);
      }
    }
}


void vtkOpenGLPolyDataMapper::ReplaceShaderValues(std::string &VSSource,
                                                  std::string &FSSource,
                                                  std::string &GSSource,
                                                  int lightComplexity, vtkRenderer* ren, vtkActor *actor)
{
  // handle colors / materials
  this->ReplaceShaderColorMaterialValues(VSSource, FSSource, GSSource, lightComplexity, ren, actor);

  // do we need the vertex in the shader in View Coordinates
  if (lightComplexity > 0)
    {
    substitute(VSSource,
      "//VTK::PositionVC::Dec",
      "varying vec4 vertexVC;");
    substitute(VSSource,
      "//VTK::PositionVC::Impl",
      "vertexVC = MCVCMatrix * vertexMC;\n"
      "  gl_Position = MCDCMatrix * vertexMC;\n");
    if (substitute(VSSource,
        "//VTK::Camera::Dec",
        "uniform mat4 MCDCMatrix;\n"
        "uniform mat4 MCVCMatrix;"))
      {
      this->ShaderVariablesUsed.push_back("MCVCMatrix");
      }
    substitute(FSSource,
      "//VTK::PositionVC::Dec",
      "varying vec4 vertexVC;");
    }
  else
    {
    substitute(VSSource,
      "//VTK::Camera::Dec",
      "uniform mat4 MCDCMatrix;");
    substitute(VSSource,
      "//VTK::PositionVC::Impl",
      "  gl_Position = MCDCMatrix * vertexMC;\n");
    }

  // normals?
  if (this->Layout.NormalOffset)
    {
    if (substitute(VSSource,
      "//VTK::Normal::Dec",
      "attribute vec3 normalMC;\n"
      "uniform mat3 normalMatrix;\n"
      "varying vec3 normalVCVarying;"))
      {
      this->ShaderVariablesUsed.push_back("normalMatrix");
      }
    substitute(VSSource,
      "//VTK::Normal::Impl",
      "normalVCVarying = normalMatrix * normalMC;");
    substitute(FSSource,
      "//VTK::Normal::Dec",
      "varying vec3 normalVCVarying;");
    substitute(FSSource,
      "//VTK::Normal::Impl",
      "vec3 normalVC = normalize(normalVCVarying);\n"
      //  if (!gl_FrontFacing) does not work in intel hd4000 mac
      //  if (int(gl_FrontFacing) == 0) does not work on mesa
      "  if (gl_FrontFacing == false) { normalVC = -normalVC; }\n"
      //"normalVC = normalVCVarying;"
      );
    }
  else
    {
    substitute(FSSource,"//VTK::System::Dec",
      "#ifdef GL_ES\n"
      "#extension GL_OES_standard_derivatives : enable\n"
      "#endif\n"
      "//VTK::System::Dec\n",
      false);
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
      substitute(FSSource,"//VTK::Normal::Impl",
        "vec3 normalVC;\n"
        "  vec3 fdx = normalize(vec3(dFdx(vertexVC.x),dFdx(vertexVC.y),dFdx(vertexVC.z)));\n"
        "  vec3 fdy = normalize(vec3(dFdy(vertexVC.x),dFdy(vertexVC.y),dFdy(vertexVC.z)));\n"
        "  if (abs(fdx.x) > 0.0)\n"
        "    { normalVC = normalize(cross(vec3(fdx.y, -fdx.x, 0.0), fdx)); }\n"
        "  else { normalVC = normalize(cross(vec3(fdy.y, -fdy.x, 0.0), fdy));}"
        );
      }
    else
      {
      substitute(FSSource,"//VTK::Normal::Impl",
        "vec3 fdx = normalize(vec3(dFdx(vertexVC.x),dFdx(vertexVC.y),dFdx(vertexVC.z)));\n"
        "  vec3 fdy = normalize(vec3(dFdy(vertexVC.x),dFdy(vertexVC.y),dFdy(vertexVC.z)));\n"
        "  vec3 normalVC = normalize(cross(fdx,fdy));\n"
        // the code below is faster, but does not work on some devices
        //"vec3 normalVC = normalize(cross(dFdx(vertexVC.xyz), dFdy(vertexVC.xyz)));\n"
        "  if (normalVC.z < 0.0) { normalVC = -1.0*normalVC; }"
        );
      }
    }
  if (this->Layout.TCoordComponents)
    {
    vtkInformation *info = actor->GetPropertyKeys();
    if (info && info->Has(vtkProp::GeneralTextureTransform()))
      {
      substitute(VSSource, "//VTK::TCoord::Dec",
        "//VTK::TCoord::Dec\n"
        "uniform mat4 tcMatrix;",
        false);
      if (this->Layout.TCoordComponents == 1)
        {
        substitute(VSSource, "//VTK::TCoord::Impl",
          "vec4 tcoordTmp = tcMatrix*vec4(tcoordMC,0.0,0.0,1.0);\n"
          "tcoordVC = tcoordTmp.x/tcoordTmp.w;");
        }
      else
        {
        substitute(VSSource, "//VTK::TCoord::Impl",
          "vec4 tcoordTmp = tcMatrix*vec4(tcoordMC,0.0,1.0);\n"
          "tcoordVC = tcoordTmp.xy/tcoordTmp.w;");
        }
      }
    else
      {
      substitute(VSSource, "//VTK::TCoord::Impl",
        "tcoordVC = tcoordMC;");
      }

    if (this->Layout.TCoordComponents == 1)
      {
      substitute(VSSource, "//VTK::TCoord::Dec",
        "attribute float tcoordMC; varying float tcoordVC;");
      substitute(FSSource, "//VTK::TCoord::Dec",
        "varying float tcoordVC; uniform sampler2D texture1;");
      substitute(FSSource, "//VTK::TCoord::Impl",
        "gl_FragColor = clamp(gl_FragColor,0.0,1.0)*texture2D(texture1, vec2(tcoordVC,0.0));");
    }
    else
      {
      substitute(VSSource, "//VTK::TCoord::Dec",
        "attribute vec2 tcoordMC; varying vec2 tcoordVC;");
      substitute(FSSource, "//VTK::TCoord::Dec",
        "varying vec2 tcoordVC; uniform sampler2D texture1;");
      // do texture mapping except for scalar coloring case which is
      // handled above
      if (!this->InterpolateScalarsBeforeMapping || !this->ColorCoordinates)
        {
        substitute(FSSource, "//VTK::TCoord::Impl",
          "gl_FragColor = clamp(gl_FragColor,0.0,1.0)*texture2D(texture1, tcoordVC.st);");
        }
      }
    }


  vtkHardwareSelector* selector = ren->GetSelector();
  bool picking = (ren->GetRenderWindow()->GetIsPicking() || selector != NULL);
  if (picking)
    {
    // technically with OpenGL 2.1 you need an extension to make use of gl_PrimitiveId
    // so we request the shader4 extension.  This is particularly useful for apple
    // systems that do not provide gl_PrimitiveId otherwise.
    // we put this before the System Declarations
    substitute(FSSource,"//VTK::System::Dec",
      "#extension GL_EXT_gpu_shader4 : enable\n"
      "//VTK::System::Dec\n",
      false);
    if (substitute(FSSource, "//VTK::Picking::Dec",
        "uniform vec3 mapperIndex;\n"
        "uniform int pickingAttributeIDOffset;"))
      {
      this->ShaderVariablesUsed.push_back("pickingAttributeIDOffset");
      }
    substitute(FSSource, "//VTK::Picking::Impl",
      "if (mapperIndex == vec3(0.0,0.0,0.0))\n"
      "    {\n"
      "    int idx = gl_PrimitiveID + 1 + pickingAttributeIDOffset;\n"
      "    gl_FragColor = vec4(float(idx%256)/255.0, float((idx/256)%256)/255.0, float(idx/65536)/255.0, 1.0);\n"
      "    }\n"
      "  else\n"
      "    {\n"
      "    gl_FragColor = vec4(mapperIndex,1.0);\n"
      "    }");
    }

  if (ren->GetLastRenderingUsedDepthPeeling())
    {
    substitute(FSSource, "//VTK::DepthPeeling::Dec",
      "uniform vec2 screenSize;\n"
      "uniform sampler2D opaqueZTexture;\n"
      "uniform sampler2D translucentZTexture;\n");
    substitute(FSSource, "//VTK::DepthPeeling::Impl",
      "float odepth = texture2D(opaqueZTexture, gl_FragCoord.xy/screenSize).r;\n"
      "  if (gl_FragCoord.z >= odepth) { discard; }\n"
      "  float tdepth = texture2D(translucentZTexture, gl_FragCoord.xy/screenSize).r;\n"
      "  if (gl_FragCoord.z <= tdepth) { discard; }\n"
      //  "gl_FragColor = vec4(odepth*odepth,tdepth*tdepth,gl_FragCoord.z*gl_FragCoord.z,1.0);"
      );
    }

  if (this->GetNumberOfClippingPlanes())
    {
    // add all the clipping planes
    int numClipPlanes = this->GetNumberOfClippingPlanes();
    if (numClipPlanes > 6)
      {
      vtkErrorMacro(<< "OpenGL has a limit of 6 clipping planes");
      numClipPlanes = 6;
      }

    substitute(VSSource, "//VTK::Clip::Dec",
      "uniform int numClipPlanes;\n"
      "uniform vec4 clipPlanes[6];\n"
      "varying float clipDistances[6];");
    substitute(VSSource, "//VTK::Clip::Impl",
      "for (int planeNum = 0; planeNum < numClipPlanes; planeNum++)\n"
      "    {\n"
      "    clipDistances[planeNum] = dot(clipPlanes[planeNum], vertexMC);\n"
      "    }\n");
    substitute(FSSource, "//VTK::Clip::Dec",
      "uniform int numClipPlanes;\n"
      "varying float clipDistances[6];");
    substitute(FSSource, "//VTK::Clip::Impl",
      "for (int planeNum = 0; planeNum < numClipPlanes; planeNum++)\n"
      "    {\n"
      "    if (clipDistances[planeNum] < 0.0) discard;\n"
      "    }\n");
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
  bool haveNormals = (this->CurrentInput->GetPointData()->GetNormals() != NULL);
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
    lightComplexity = 0;
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
        if (lightComplexity == 0)
          {
          lightComplexity = 1;
          }
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
      cellBO.ShaderSourceTime < this->CurrentInput->GetMTime() ||
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

  this->SetMapperShaderParameters(cellBO, ren, actor);
  this->SetPropertyShaderParameters(cellBO, ren, actor);
  this->SetCameraShaderParameters(cellBO, ren, actor);
  this->SetLightingShaderParameters(cellBO, ren, actor);
  cellBO.vao.Bind();

  this->LastBoundBO = &cellBO;
}

void vtkOpenGLPolyDataMapper::SetMapperShaderParameters(vtkgl::CellBO &cellBO,
                                                      vtkRenderer* ren, vtkActor *actor)
{
  // Now to update the VAO too, if necessary.
  vtkgl::VBOLayout &layout = this->Layout;

  if (cellBO.indexCount && (this->VBOBuildTime > cellBO.attributeUpdateTime ||
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

    // check for tcoord transform matrix
    vtkInformation *info = actor->GetPropertyKeys();
    vtkOpenGLCheckErrorMacro("failed after Render");
    if (info && info->Has(vtkProp::GeneralTextureTransform()))
      {
      double *dmatrix = info->Get(vtkProp::GeneralTextureTransform());
      float fmatrix[16];
      for (int i = 0; i < 4; i++)
        {
        for (int j = 0; j < 4; j++)
          {
          fmatrix[j*4+i] = dmatrix[i*4+j];
          }
        }
      cellBO.Program->SetUniformMatrix4x4("tcMatrix", fmatrix);
      vtkOpenGLCheckErrorMacro("failed after Render");
      }
    }

  // if depth peeling set the required uniforms
  if (ren->GetLastRenderingUsedDepthPeeling())
    {
    // check for prop keys
    vtkInformation *info = actor->GetPropertyKeys();
    if (info && info->Has(vtkDepthPeelingPass::OpaqueZTextureUnit()) &&
        info->Has(vtkDepthPeelingPass::TranslucentZTextureUnit()))
      {
      int otunit = info->Get(vtkDepthPeelingPass::OpaqueZTextureUnit());
      int ttunit = info->Get(vtkDepthPeelingPass::TranslucentZTextureUnit());
      cellBO.Program->SetUniformi("opaqueZTexture", otunit);
      cellBO.Program->SetUniformi("translucentZTexture", ttunit);

      int *renSize = info->Get(vtkDepthPeelingPass::DestinationSize());
      float screenSize[2];
      screenSize[0] = renSize[0];
      screenSize[1] = renSize[1];
      cellBO.Program->SetUniform2f("screenSize", screenSize);
      }
    }

  vtkHardwareSelector* selector = ren->GetSelector();
  bool picking = (ren->GetRenderWindow()->GetIsPicking() || selector != NULL);
  if (picking)
    {
    if (this->IsShaderVariableUsed("pickingAttributeIDOffset"))
      {
      cellBO.Program->SetUniformi("pickingAttributeIDOffset",
        this->pickingAttributeIDOffset);
      }
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

  if (this->GetNumberOfClippingPlanes())
    {
    // add all the clipping planes
    int numClipPlanes = this->GetNumberOfClippingPlanes();
    if (numClipPlanes > 6)
      {
      vtkErrorMacro(<< "OpenGL has a limit of 6 clipping planes");
      numClipPlanes = 6;
      }

    float planeEquations[6][4];
    for (int i = 0; i < numClipPlanes; i++)
      {
      double planeEquation[4];
      this->GetClippingPlaneInDataCoords(actor->GetMatrix(), i, planeEquation);
      planeEquations[i][0] = planeEquation[0];
      planeEquations[i][1] = planeEquation[1];
      planeEquations[i][2] = planeEquation[2];
      planeEquations[i][3] = planeEquation[3];
      }
    cellBO.Program->SetUniformi("numClipPlanes", numClipPlanes);
    cellBO.Program->SetUniform4fv("clipPlanes", 6, planeEquations);
    }
}

//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::SetLightingShaderParameters(vtkgl::CellBO &cellBO,
                                                      vtkRenderer* ren, vtkActor *vtkNotUsed(actor))
{
  // for unlit and headlight there are no lighting parameters
  if (this->LastLightComplexity < 2 || this->DrawingEdges)
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
  float lightHalfAngle[6][3];
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
      lightDir[0] = -tDir[0];
      lightDir[1] = -tDir[1];
      lightDir[2] = -tDir[2]+1.0;
      vtkMath::Normalize(lightDir);
      lightHalfAngle[numberOfLights][0] = lightDir[0];
      lightHalfAngle[numberOfLights][1] = lightDir[1];
      lightHalfAngle[numberOfLights][2] = lightDir[2];
      numberOfLights++;
      }
    }

  program->SetUniform3fv("lightColor", numberOfLights, lightColor);
  program->SetUniform3fv("lightDirectionVC", numberOfLights, lightDirection);
  program->SetUniform3fv("lightHalfAngleVC", numberOfLights, lightHalfAngle);
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

  vtkOpenGLCamera *cam = (vtkOpenGLCamera *)(ren->GetActiveCamera());

  vtkMatrix4x4 *wcdc;
  vtkMatrix4x4 *wcvc;
  vtkMatrix3x3 *norms;
  vtkMatrix4x4 *vcdc;
  cam->GetKeyMatrices(ren,wcvc,norms,vcdc,wcdc);

  if (!actor->GetIsIdentity())
    {
    vtkMatrix4x4 *mcwc;
    vtkMatrix3x3 *anorms;
    ((vtkOpenGLActor *)actor)->GetKeyMatrices(mcwc,anorms);
    vtkMatrix4x4::Multiply4x4(mcwc, wcdc, this->TempMatrix4);
    program->SetUniformMatrix("MCDCMatrix", this->TempMatrix4);
    if (this->IsShaderVariableUsed("MCVCMatrix"))
      {
      vtkMatrix4x4::Multiply4x4(mcwc, wcvc, this->TempMatrix4);
      program->SetUniformMatrix("MCVCMatrix", this->TempMatrix4);
      }
    if (this->IsShaderVariableUsed("normalMatrix"))
      {
      vtkMatrix3x3::Multiply3x3(anorms, norms, this->TempMatrix3);
      program->SetUniformMatrix("normalMatrix", this->TempMatrix3);
      }
    }
  else
    {
    program->SetUniformMatrix("MCDCMatrix", wcdc);
    if (this->IsShaderVariableUsed("MCVCMatrix"))
      {
      program->SetUniformMatrix("MCVCMatrix", wcvc);
      }
    if (this->IsShaderVariableUsed("normalMatrix"))
      {
      program->SetUniformMatrix("normalMatrix", norms);
      }
    }
}

//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::SetPropertyShaderParameters(vtkgl::CellBO &cellBO,
                                                       vtkRenderer*, vtkActor *actor)
{
  vtkShaderProgram *program = cellBO.Program;

  vtkProperty *ppty = actor->GetProperty();

  {
  // Query the property for some of the properties that can be applied.
  float opacity = static_cast<float>(ppty->GetOpacity());
  double *aColor = this->DrawingEdges ?
    ppty->GetEdgeColor() : ppty->GetAmbientColor();
  double aIntensity = this->DrawingEdges ? 1.0 : ppty->GetAmbient();
  float ambientColor[3] = {static_cast<float>(aColor[0] * aIntensity),
    static_cast<float>(aColor[1] * aIntensity),
    static_cast<float>(aColor[2] * aIntensity)};
  double *dColor = ppty->GetDiffuseColor();
  double dIntensity = this->DrawingEdges ? 0.0 : ppty->GetDiffuse();
  float diffuseColor[3] = {static_cast<float>(dColor[0] * dIntensity),
    static_cast<float>(dColor[1] * dIntensity),
    static_cast<float>(dColor[2] * dIntensity)};
  double *sColor = ppty->GetSpecularColor();
  double sIntensity = this->DrawingEdges ? 0.0 : ppty->GetSpecular();
  float specularColor[3] = {static_cast<float>(sColor[0] * sIntensity),
    static_cast<float>(sColor[1] * sIntensity),
    static_cast<float>(sColor[2] * sIntensity)};
  double specularPower = ppty->GetSpecularPower();

  program->SetUniformf("opacityUniform", opacity);
  program->SetUniform3f("ambientColorUniform", ambientColor);
  program->SetUniform3f("diffuseColorUniform", diffuseColor);
  // we are done unless we have lighting
  if (this->LastLightComplexity < 1)
    {
    return;
    }
  program->SetUniform3f("specularColorUniform", specularColor);
  program->SetUniformf("specularPowerUniform", specularPower);
  }

  // now set the backface properties if we have them
  if (actor->GetBackfaceProperty() && !this->DrawingEdges)
    {
    ppty = actor->GetBackfaceProperty();

    float opacity = static_cast<float>(ppty->GetOpacity());
    double *aColor = ppty->GetAmbientColor();
    double aIntensity = ppty->GetAmbient();  // ignoring renderer ambient
    float ambientColor[3] = {static_cast<float>(aColor[0] * aIntensity),
      static_cast<float>(aColor[1] * aIntensity),
      static_cast<float>(aColor[2] * aIntensity)};
    double *dColor = ppty->GetDiffuseColor();
    double dIntensity = ppty->GetDiffuse();
    float diffuseColor[3] = {static_cast<float>(dColor[0] * dIntensity),
      static_cast<float>(dColor[1] * dIntensity),
      static_cast<float>(dColor[2] * dIntensity)};
    double *sColor = ppty->GetSpecularColor();
    double sIntensity = ppty->GetSpecular();
    float specularColor[3] = {static_cast<float>(sColor[0] * sIntensity),
      static_cast<float>(sColor[1] * sIntensity),
      static_cast<float>(sColor[2] * sIntensity)};
    double specularPower = ppty->GetSpecularPower();

    program->SetUniformf("opacityUniformBF", opacity);
    program->SetUniform3f("ambientColorUniformBF", ambientColor);
    program->SetUniform3f("diffuseColorUniformBF", diffuseColor);
    // we are done unless we have lighting
    if (this->LastLightComplexity < 1)
      {
      return;
      }
    program->SetUniform3f("specularColorUniformBF", specularColor);
    program->SetUniformf("specularPowerUniformBF", specularPower);
    }

}

//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::RenderPieceStart(vtkRenderer* ren, vtkActor *actor)
{
  // Set the PointSize and LineWidget
#if GL_ES_VERSION_2_0 != 1
  glPointSize(actor->GetProperty()->GetPointSize()); // not on ES2
#endif
  glLineWidth(actor->GetProperty()->GetLineWidth()); // supported by all OpenGL versions

  vtkHardwareSelector* selector = ren->GetSelector();
  if (selector && this->PopulateSelectionSettings)
    {
    selector->BeginRenderProp();
    // render points for point picking in a special way
    if (selector->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_POINTS &&
        selector->GetCurrentPass() > vtkHardwareSelector::ACTOR_PASS)
      {
#if GL_ES_VERSION_2_0 != 1
      glPointSize(4.0); //make verts large enough to be sure to overlap cell
#endif
      glEnable(GL_POLYGON_OFFSET_FILL);
      glPolygonOffset(0,2.0);  // supported on ES2/3/etc
      glDepthMask(GL_FALSE); //prevent verts from interfering with each other
      }
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

  // make sure the BOs are up to date
  this->UpdateBufferObjects(ren, actor);

  // If we are coloring by texture, then load the texture map.
  // Use Map as indicator, because texture hangs around.
  if (this->InternalColorTexture)
    {
    this->InternalColorTexture->Load(ren);
    }

  // Bind the OpenGL, this is shared between the different primitive/cell types.
  this->VBO.Bind();

  this->LastBoundBO = NULL;

  vtkProperty *prop = actor->GetProperty();
  bool draw_surface_with_edges =
    (prop->GetEdgeVisibility() && prop->GetRepresentation() == VTK_SURFACE);

  if ( this->GetResolveCoincidentTopology() || draw_surface_with_edges)
    {
    glEnable(GL_POLYGON_OFFSET_FILL);
    if ( this->GetResolveCoincidentTopology() == VTK_RESOLVE_SHIFT_ZBUFFER )
      {
      // do something rough is better than nothing
      double zRes = this->GetResolveCoincidentTopologyZShift(); // 0 is no shift 1 is big shift
      double f = zRes*4.0;
      glPolygonOffset(f + (draw_surface_with_edges ? 1.0 : 0.0),
        draw_surface_with_edges ? 1.0 : 0.0);  // supported on ES2/3/etc
      }
    else
      {
      double f, u;
      this->GetResolveCoincidentTopologyPolygonOffsetParameters(f,u);
      glPolygonOffset(f + (draw_surface_with_edges ? 1.0 : 0.0),
        u + (draw_surface_with_edges ? 1.0 : 0.0));  // supported on ES2/3/etc
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

  int representation = actor->GetProperty()->GetRepresentation();

  // render points for point picking in a special way
  // all cell types should be rendered as points
  vtkHardwareSelector* selector = ren->GetSelector();
  if (selector && this->PopulateSelectionSettings &&
      selector->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_POINTS &&
      selector->GetCurrentPass() > vtkHardwareSelector::ACTOR_PASS)
    {
    representation = VTK_POINTS;
    }

  // draw lines
  if (this->Lines.indexCount)
    {
    this->UpdateShader(this->Lines, ren, actor);
    this->Lines.ibo.Bind();
    if (representation == VTK_POINTS)
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
    this->UpdateShader(this->Tris, ren, actor);
    this->Tris.ibo.Bind();
    GLenum mode = (representation == VTK_POINTS) ? GL_POINTS :
      (representation == VTK_WIREFRAME) ? GL_LINES : GL_TRIANGLES;
    glDrawRangeElements(mode, 0,
                      static_cast<GLuint>(layout.VertexCount - 1),
                      static_cast<GLsizei>(this->Tris.indexCount),
                      GL_UNSIGNED_INT,
                      reinterpret_cast<const GLvoid *>(NULL));
    this->Tris.ibo.Release();
    this->pickingAttributeIDOffset += (int)this->Tris.indexCount;
    }

  // draw strips
  if (this->TriStrips.indexCount)
    {
    // Use the tris shader program/VAO, but triStrips ibo.
    this->UpdateShader(this->TriStrips, ren, actor);
    this->TriStrips.ibo.Bind();
    if (representation == VTK_POINTS)
      {
      glDrawRangeElements(GL_POINTS, 0,
                          static_cast<GLuint>(layout.VertexCount - 1),
                          static_cast<GLsizei>(this->TriStrips.indexCount),
                          GL_UNSIGNED_INT,
                          reinterpret_cast<const GLvoid *>(NULL));
      }
    if (representation == VTK_WIREFRAME)
      {
      glMultiDrawElements(GL_LINE_STRIP,
                        (GLsizei *)(&this->TriStrips.elementsArray[0]),
                        GL_UNSIGNED_INT,
                        reinterpret_cast<const GLvoid **>(&(this->TriStrips.offsetArray[0])),
                        (GLsizei)this->TriStrips.offsetArray.size());
      }
    if (representation == VTK_SURFACE)
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
    // render points for point picking in a special way
    if (selector->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_POINTS &&
        selector->GetCurrentPass() > vtkHardwareSelector::ACTOR_PASS)
      {
      glDepthMask(GL_TRUE);
      glDisable(GL_POLYGON_OFFSET_FILL);
      }
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

  this->CurrentInput = this->GetInput();

  if (this->CurrentInput == NULL)
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
  if (!this->CurrentInput->GetPoints())
    {
    return;
    }

  this->RenderPieceStart(ren, actor);
  this->RenderPieceDraw(ren, actor);
  this->RenderEdges(ren,actor);
  this->RenderPieceFinish(ren, actor);
}

void vtkOpenGLPolyDataMapper::RenderEdges(vtkRenderer* ren, vtkActor *actor)
{
  vtkProperty *prop = actor->GetProperty();
  bool draw_surface_with_edges =
    (prop->GetEdgeVisibility() && prop->GetRepresentation() == VTK_SURFACE);

  if (!draw_surface_with_edges)
    {
    return;
    }

  vtkgl::VBOLayout &layout = this->Layout;
  this->DrawingEdges = true;

  // draw polygons
  if (this->TrisEdges.indexCount)
    {
    // First we do the triangles, update the shader, set uniforms, etc.
    this->UpdateShader(this->TrisEdges, ren, actor);
    this->TrisEdges.ibo.Bind();
    glDrawRangeElements(GL_LINES, 0,
                        static_cast<GLuint>(layout.VertexCount - 1),
                        static_cast<GLsizei>(this->TrisEdges.indexCount),
                        GL_UNSIGNED_INT,
                        reinterpret_cast<const GLvoid *>(NULL));
    this->TrisEdges.ibo.Release();
    }

  // draw strips
  if (this->TriStripsEdges.indexCount)
    {
    // Use the tris shader program/VAO, but triStrips ibo.
    this->UpdateShader(this->TriStripsEdges, ren, actor);
    this->TriStripsEdges.ibo.Bind();
    glMultiDrawElements(GL_LINE_STRIP,
                      (GLsizei *)(&this->TriStripsEdges.elementsArray[0]),
                      GL_UNSIGNED_INT,
                      reinterpret_cast<const GLvoid **>(&(this->TriStripsEdges.offsetArray[0])),
                      (GLsizei)this->TriStripsEdges.offsetArray.size());
    this->TriStripsEdges.ibo.Release();
    }

  this->DrawingEdges = false;

/*
    // Disable textures when rendering the surface edges.
    // This ensures that edges are always drawn solid.
    glDisable(GL_TEXTURE_2D);

    this->Information->Set(vtkPolyDataPainter::DISABLE_SCALAR_COLOR(), 1);
    this->Information->Remove(vtkPolyDataPainter::DISABLE_SCALAR_COLOR());
    */
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
void vtkOpenGLPolyDataMapper::UpdateBufferObjects(vtkRenderer *ren, vtkActor *act)
{
  if (this->GetNeedToRebuildBufferObjects(ren,act))
    {
    this->BuildBufferObjects(ren,act);
    this->VBOBuildTime.Modified();
    }
}

//-------------------------------------------------------------------------
bool vtkOpenGLPolyDataMapper::GetNeedToRebuildBufferObjects(vtkRenderer *ren, vtkActor *act)
{
  if (this->VBOBuildTime < this->GetMTime() ||
      this->VBOBuildTime < act->GetMTime() ||
      this->VBOBuildTime < this->CurrentInput->GetMTime())
    {
    return true;
    }
  else
    {
    vtkHardwareSelector* selector = ren->GetSelector();
    bool picking = (ren->GetIsPicking() || selector != NULL);
    if ((this->LastSelectionState || picking) && selector &&
          selector->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_POINTS)
      {
      return true;
      }
    }
  return false;
}

//-------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::BuildBufferObjects(vtkRenderer *ren, vtkActor *act)
{
  vtkPolyData *poly = this->CurrentInput;

  if (poly == NULL)
    {
    return;
    }

  // For vertex coloring, this sets this->Colors as side effect.
  // For texture map coloring, this sets ColorCoordinates
  // and ColorTextureMap as a side effect.
  // I moved this out of the conditional because it is fast.
  // Color arrays are cached. If nothing has changed,
  // then the scalars do not have to be regenerted.
  this->MapScalars(1.0);

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

  // Build the VBO
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

  // now create the IBOs
  this->Points.indexCount = CreatePointIndexBuffer(prims[0],
                                                   this->Points.ibo);

  int representation = act->GetProperty()->GetRepresentation();

  vtkHardwareSelector* selector = ren->GetSelector();
  if (selector && this->PopulateSelectionSettings &&
      selector->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_POINTS &&
      selector->GetCurrentPass() > vtkHardwareSelector::ACTOR_PASS)
    {
    representation = VTK_POINTS;
    }

  if (representation == VTK_POINTS)
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

    if (representation == VTK_WIREFRAME)
      {
      vtkDataArray *ef = poly->GetPointData()->GetAttribute(
                        vtkDataSetAttributes::EDGEFLAG);
      if (ef)
        {
        if (ef->GetNumberOfComponents() != 1)
          {
          vtkDebugMacro(<< "Currently only 1d edge flags are supported.");
          ef = NULL;
          }
        if (!ef->IsA("vtkUnsignedCharArray"))
          {
          vtkDebugMacro(<< "Currently only unsigned char edge flags are suported.");
          ef = NULL;
          }
        }
      if (ef)
        {
        this->Tris.indexCount = CreateEdgeFlagIndexBuffer(prims[2],
                                             this->Tris.ibo, ef);
        }
      else
        {
        this->Tris.indexCount = CreateTriangleLineIndexBuffer(prims[2],
                                           this->Tris.ibo);
        }
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

  // when drawing edges also build the edge IBOs
  vtkProperty *prop = act->GetProperty();
  bool draw_surface_with_edges =
    (prop->GetEdgeVisibility() && prop->GetRepresentation() == VTK_SURFACE);
  if (draw_surface_with_edges)
    {
    vtkDataArray *ef = poly->GetPointData()->GetAttribute(
                        vtkDataSetAttributes::EDGEFLAG);
    if (ef)
      {
      if (ef->GetNumberOfComponents() != 1)
        {
        vtkDebugMacro(<< "Currently only 1d edge flags are supported.");
        ef = NULL;
        }
      if (!ef->IsA("vtkUnsignedCharArray"))
        {
        vtkDebugMacro(<< "Currently only unsigned char edge flags are suported.");
        ef = NULL;
        }
      }
    if (ef)
      {
      this->TrisEdges.indexCount = CreateEdgeFlagIndexBuffer(prims[2],
                                           this->TrisEdges.ibo, ef);
      }
    else
      {
      this->TrisEdges.indexCount = CreateTriangleLineIndexBuffer(prims[2],
                                           this->TrisEdges.ibo);
      }
    this->TriStripsEdges.indexCount = CreateMultiIndexBuffer(prims[3],
                         this->TriStripsEdges.ibo,
                         this->TriStripsEdges.offsetArray,
                         this->TriStripsEdges.elementsArray, true);
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
      (this->ColorMode == VTK_COLOR_MODE_DEFAULT ||
       this->ColorMode == VTK_COLOR_MODE_DIRECT_SCALARS))
    {
    vtkPolyData* input =
      vtkPolyData::SafeDownCast(this->GetInputDataObject(0, 0));
    if (input)
      {
      int cellFlag;
      vtkDataArray* scalars = this->GetScalars(input,
        this->ScalarMode, this->ArrayAccessMode, this->ArrayId,
        this->ArrayName, cellFlag);
      if (scalars &&
          (scalars->IsA("vtkUnsignedCharArray") ||
           this->ColorMode == VTK_COLOR_MODE_DIRECT_SCALARS) &&
        (scalars->GetNumberOfComponents() ==  4 /*(RGBA)*/ ||
         scalars->GetNumberOfComponents() == 2 /*(LuminanceAlpha)*/))
        {
        int opacityIndex = scalars->GetNumberOfComponents() - 1;
        unsigned char opacity = 0;
        switch (scalars->GetDataType())
          {
          vtkTemplateMacro(
            vtkScalarsToColors::ColorToUChar(
              static_cast<VTK_TT>(scalars->GetRange(opacityIndex)[0]),
              &opacity));
          }
        if (opacity < 255)
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

vtkIdType vtkOpenGLPolyDataMapper::GetConvertedPickValue(vtkIdType idIn, int fieldassociation, vtkActor *act)
{
  vtkPolyData *poly = this->CurrentInput;
  vtkCellArray *prims[4];
  prims[0] =  poly->GetVerts();
  prims[1] =  poly->GetLines();
  prims[2] =  poly->GetPolys();
  prims[3] =  poly->GetStrips();

  vtkIdType* indices(NULL);
  vtkIdType npts(0);
  vtkIdType localId = idIn;

  // handle cell picking
  if (fieldassociation == vtkDataObject::FIELD_ASSOCIATION_CELLS)
    {
    // for points the cell is the cell, easy peasy
    if (static_cast<size_t>(localId) < this->Points.indexCount)
      {
      return localId;
      }
    localId -= this->Points.indexCount;
    vtkIdType offset = 0;  // adjustment between OpenGL cells and VTK cells

    int representation = act->GetProperty()->GetRepresentation();

    // for lines the cell has to be computed because we do not
    // know how many line segments are in the polyline
    if (this->Lines.indexCount > 0)
      {
      // compute the location in the cell array
      vtkIdType cellCount = 0;
      for (prims[1]->InitTraversal(); prims[1]->GetNextCell(npts, indices); )
        {
        vtkIdType numCells = (representation == VTK_POINTS) ? npts : (npts - 1);
        if (localId < cellCount + numCells)
          {
          return idIn + offset + cellCount - localId;
          }
        offset = offset + 1 - numCells;
        cellCount += numCells;
        }
      localId -= (this->Lines.indexCount/2);
      }

    // for polys the cell has to be computed because we do not
    // know how many triangles are in the poly
    if (this->Tris.indexCount > 0)
      {
      // compute the location in the cell array
      vtkIdType cellCount = 0;
      for (prims[2]->InitTraversal(); prims[2]->GetNextCell(npts, indices); )
        {
        vtkIdType numCells = (representation == VTK_POINTS) ? npts :
          (representation == VTK_WIREFRAME) ? npts : (npts - 2);
        if (localId < cellCount + numCells)
          {
          return idIn + offset + cellCount - localId;
          }
        offset = offset + 1 - numCells;
        cellCount += numCells;
        }
      localId -= (this->Tris.indexCount/3);
      }

    // for strips the cell maps exactly, easy peasy
    if (static_cast<size_t>(localId) < this->TriStrips.indexCount)
      {
      return idIn + offset;
      }
    return 0;
    }

  // if we got here, then it is point based picking
  // is it a point?
   if (static_cast<size_t>(localId) < this->Points.indexCount)
    {
    prims[0]->GetCell(localId,npts,indices);
    return indices[0];
    }
  localId -= this->Points.indexCount;

  // when picking in point mode, we render all primitives as
  // points.  The graphics hardware tells us what point was
  // picked.  e.g. the 11th point. We have to convert that into
  // a point ID. This can be done by traversing the cell
  // arrays to find the 11th point rendered. But that operation
  // can be expensive for the more complex cell arrays.
  // You could speed this up significantly by building a
  // monotonically increasing array of indexes that are
  // not points in the cells array.  Then a binary search
  // would let you index quickly into the right array location.

  // is it a line?
  if (static_cast<size_t>(localId) <  this->Lines.indexCount)
    {
    // compute the location in the cell array
    vtkIdType pointCount = 0;
    for (prims[1]->InitTraversal(); prims[1]->GetNextCell(npts, indices); )
      {
      if (localId < pointCount + npts)
        {
        return indices[localId - pointCount];
        }
      pointCount += npts;
      }
    }
  localId -= this->Lines.indexCount;

  // is it a poly
  if (static_cast<size_t>(localId) <  this->Tris.indexCount)
    {
    // compute the location in the cell array
    vtkIdType pointCount = 0;
    for (prims[2]->InitTraversal(); prims[2]->GetNextCell(npts, indices); )
      {
      if (localId < pointCount + npts)
        {
        return indices[localId - pointCount];
        }
      pointCount += npts;
      }
    }
  localId -= this->Tris.indexCount;

  // is it a strip?
  if (static_cast<size_t>(localId) <  this->TriStrips.indexCount)
    {
    // compute the location in the cell array
    vtkIdType pointCount = 0;
    for (prims[3]->InitTraversal(); prims[3]->GetNextCell(npts, indices); )
      {
      if (localId < pointCount + npts)
        {
        return indices[localId - pointCount];
        }
      pointCount += npts;
      }
    }

  return idIn;
}

//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
