// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDGOpenGLRenderer.h"

#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkCellMetadata.h"
#include "vtkDGCell.h"
#include "vtkDataSetAttributes.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkIntArray.h"
#include "vtkLightingMapPass.h"
#include "vtkMatrix3x3.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLActor.h"
#include "vtkOpenGLBufferObject.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLCellGridMapper.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLHelper.h"
#include "vtkOpenGLIndexBufferObject.h"
#include "vtkOpenGLRenderPass.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLState.h"
#include "vtkOpenGLTexture.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkProperty.h"
#include "vtkShader.h"
#include "vtkShaderProgram.h"
#include "vtkTextureObject.h"
#include "vtkTypeFloat32Array.h"
#include "vtkTypeInt32Array.h"

#include "vtkCellGridFS_DGHex.h"
#include "vtkCellGridFS_DGTet.h"
#include "vtkCellGridGS_DGHex.h"
#include "vtkCellGridGS_DGTet.h"
#include "vtkCellGridVS.h"

#include <numeric> // for iota
#include <vector>  // for sideIndices

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

vtkStandardNewMacro(vtkDGOpenGLRenderer);

class UploadableTexBuffer
{
public:
  vtkNew<vtkTextureObject> Texture;
  vtkNew<vtkOpenGLBufferObject> Buffer;
};

class DGState : public vtkOpenGLCellGridRenderRequest::StateBase
{
public:
  using ShaderMap = std::map<vtkShader::Type, vtkShader*>;

  DGState() = default;

  int SideShape{ -1 };
  vtkOpenGLHelper CellBO;
  vtkNew<vtkTypeFloat32Array> InputPoints;
  vtkNew<vtkTypeInt32Array> InputCells;
  vtkNew<vtkTypeInt32Array> InputSides;
  vtkSmartPointer<vtkFloatArray> ParametricCoordinates;
  vtkSmartPointer<vtkIntArray> FaceConnectivity;
  vtkNew<vtkTypeFloat32Array> InputFieldCoefficients;

  UploadableTexBuffer CellConnectivityTB;
  UploadableTexBuffer SideConnectivityTB;
  UploadableTexBuffer FaceConnectivityTB;
  UploadableTexBuffer CellParametricsTB;
  UploadableTexBuffer PointCoordinatesTB;
  UploadableTexBuffer FieldCoefficientsTB;

  vtkNew<vtkOpenGLTexture> ColorTextureGL;

  vtkTimeStamp LightComplexityChanged;
  int LastLightComplexity{ 0 };
  int LastLightCount{ 0 };

  vtkNew<vtkMatrix4x4> TempMatrix4;
  vtkNew<vtkMatrix3x3> TempMatrix3;
  vtkStringToken CellTypeToken;
  vtkStringToken ShortTypeToken;

  bool SetMetadataAndSideShape(vtkDGCell* metadata, int sideShape, vtkStringToken shortTypeToken);
  bool RebuildShadersIfNeeded(vtkOpenGLCellGridRenderRequest* request);
  void ReplaceShaderRenderPass(
    ShaderMap& shaders, vtkOpenGLCellGridRenderRequest* request, bool prePass);
  void ReplaceShaderColor(ShaderMap& shaders, vtkOpenGLCellGridRenderRequest* request);
  void ReplaceShaderNormal(ShaderMap& shaders, vtkOpenGLCellGridRenderRequest* request);
  void ReplaceShaderLight(ShaderMap& shaders, vtkOpenGLCellGridRenderRequest* request);
  // void ReplaceShaderTCoord(ShaderMap& shaders, vtkOpenGLCellGridRenderRequest* request);
  // void ReplaceShaderPicking(ShaderMap& shaders, vtkOpenGLCellGridRenderRequest* request);
  // void ReplaceShaderClip(ShaderMap& shaders, vtkOpenGLCellGridRenderRequest* request);
  void ReplaceShaderPositionVC(ShaderMap& shaders, vtkOpenGLCellGridRenderRequest* request);

  void SetMapperShaderParameters(vtkOpenGLCellGridRenderRequest* request);
  void SetPropertyShaderParameters(vtkOpenGLCellGridRenderRequest* request);
  void SetCameraShaderParameters(vtkOpenGLCellGridRenderRequest* request);
  void SetLightingShaderParameters(vtkOpenGLCellGridRenderRequest* request);
};

bool DGState::SetMetadataAndSideShape(
  vtkDGCell* metadata, int sideShape, vtkStringToken shortCellToken)
{
  this->ParametricCoordinates = metadata->GetReferencePoints();
  this->FaceConnectivity = metadata->GetSideConnectivity();
  this->SideShape = sideShape;
  std::string cellTypeName = metadata->GetClassName();
  this->CellTypeToken = cellTypeName;    // Include leading "vtk"
  this->ShortTypeToken = shortCellToken; // Without leading "vtk"
  return true;
}

bool DGState::RebuildShadersIfNeeded(vtkOpenGLCellGridRenderRequest* request)
{
  auto* actor = request->GetActor();
  auto* renderer = request->GetRenderer();

  int lightComplexity = 0;
  int numberOfLights = 0;
  bool needLighting = false;
  if (actor->GetProperty()->GetRepresentation() == VTK_POINTS)
  {
    needLighting = (actor->GetProperty()->GetInterpolation() != VTK_FLAT);
  }
  else // wireframe or surface rep
  {
    bool isTris = true;
    needLighting = (isTris || (!isTris && actor->GetProperty()->GetInterpolation() != VTK_FLAT));
  }
  // do we need lighting?
  if (actor->GetProperty()->GetLighting() && needLighting)
  {
    vtkOpenGLRenderer* oren = static_cast<vtkOpenGLRenderer*>(renderer);
    lightComplexity = oren->GetLightingComplexity();
    numberOfLights = oren->GetLightingCount();
  }

  if (this->LastLightComplexity != lightComplexity || this->LastLightCount != numberOfLights)
  {
    this->LightComplexityChanged.Modified();
    this->LastLightComplexity = lightComplexity;
    this->LastLightCount = numberOfLights;
  }

  auto oglRenWin = vtkOpenGLRenderWindow::SafeDownCast(renderer->GetRenderWindow());
  if (!!this->CellBO.Program)
  {
    oglRenWin->GetShaderCache()->ReadyShaderProgram(this->CellBO.Program);
    if (this->CellBO.Program->GetMTime() > this->CellBO.AttributeUpdateTime)
    {
      this->CellBO.VAO->ReleaseGraphicsResources(); // Reset VAO
    }
    // FIXME: We should return false whenever possible, but doing so
    //        currently causes problems when the mapper's scalar visibility
    //        is modified.
    // return false;
  }

  // OK, we need to update shaders.
  ShaderMap shaders;
  vtkShader* vss = vtkShader::New();
  vss->SetType(vtkShader::Vertex);
  shaders[vtkShader::Vertex] = vss;

  vtkShader* fss = vtkShader::New();
  fss->SetType(vtkShader::Fragment);
  shaders[vtkShader::Fragment] = fss;

  vtkShader* gss = vtkShader::New();
  gss->SetType(vtkShader::Geometry);
  shaders[vtkShader::Geometry] = gss;

  shaders[vtkShader::Vertex]->SetType(vtkShader::Vertex);
  shaders[vtkShader::Fragment]->SetType(vtkShader::Fragment);
  shaders[vtkShader::Geometry]->SetType(vtkShader::Geometry);
  shaders[vtkShader::Vertex]->SetSource(vtkCellGridVS);
  // XXX(c++14)
#if __cplusplus < 201400L
  if (this->ShortTypeToken.GetId() == "DGHex"_hash)
  {
    shaders[vtkShader::Fragment]->SetSource(vtkCellGridFS_DGHex);
    shaders[vtkShader::Geometry]->SetSource(vtkCellGridGS_DGHex);
  }
  else if (this->ShortTypeToken.GetId() == "DGTet"_hash)
  {
    shaders[vtkShader::Fragment]->SetSource(vtkCellGridFS_DGTet);
    shaders[vtkShader::Geometry]->SetSource(vtkCellGridGS_DGTet);
  }
#else
  switch (this->ShortTypeToken.GetId())
  {
    case "DGHex"_hash:
      shaders[vtkShader::Fragment]->SetSource(vtkCellGridFS_DGHex);
      shaders[vtkShader::Geometry]->SetSource(vtkCellGridGS_DGHex);
      break;
    case "DGTet"_hash:
      shaders[vtkShader::Fragment]->SetSource(vtkCellGridFS_DGTet);
      shaders[vtkShader::Geometry]->SetSource(vtkCellGridGS_DGTet);
      break;
  }
#endif

  this->ReplaceShaderRenderPass(shaders, request, true);
  this->ReplaceShaderColor(shaders, request);
  this->ReplaceShaderNormal(shaders, request);
  this->ReplaceShaderLight(shaders, request);
  // this->ReplaceShaderTCoord(shaders, request);
  // this->ReplaceShaderPicking(shaders, request);
  // this->ReplaceShaderClip(shaders, request);
  this->ReplaceShaderPositionVC(shaders, request);
  this->ReplaceShaderRenderPass(shaders, request, false);

  vtkShaderProgram* program = oglRenWin->GetShaderCache()->ReadyShaderProgram(shaders);
  vss->Delete();
  fss->Delete();
  gss->Delete();
  this->CellBO.Program = program;
  this->CellBO.VAO->ReleaseGraphicsResources();
  this->CellBO.ShaderSourceTime.Modified();
  return true;
}

void DGState::ReplaceShaderRenderPass(
  ShaderMap& shaders, vtkOpenGLCellGridRenderRequest* request, bool prePass)
{
  std::string VSSource = shaders[vtkShader::Vertex]->GetSource();
  std::string GSSource = shaders[vtkShader::Geometry]->GetSource();
  std::string FSSource = shaders[vtkShader::Fragment]->GetSource();

  auto* actor = request->GetActor();
  vtkInformation* info = actor->GetPropertyKeys();
  if (info && info->Has(vtkOpenGLRenderPass::RenderPasses()))
  {
    int numRenderPasses = info->Length(vtkOpenGLRenderPass::RenderPasses());
    for (int i = 0; i < numRenderPasses; ++i)
    {
      vtkObjectBase* rpBase = info->Get(vtkOpenGLRenderPass::RenderPasses(), i);
      vtkOpenGLRenderPass* rp = static_cast<vtkOpenGLRenderPass*>(rpBase);
      if (prePass)
      {
        if (!rp->PreReplaceShaderValues(VSSource, GSSource, FSSource, request->GetMapper(), actor))
        {
          vtkErrorWithObjectMacro(request->GetMapper(),
            "vtkOpenGLRenderPass::ReplaceShaderValues failed for " << rp->GetClassName());
        }
      }
      else
      {
        if (!rp->PostReplaceShaderValues(VSSource, GSSource, FSSource, request->GetMapper(), actor))
        {
          vtkErrorWithObjectMacro(request->GetMapper(),
            "vtkOpenGLRenderPass::ReplaceShaderValues failed for " << rp->GetClassName());
        }
      }
    }
  }
  shaders[vtkShader::Vertex]->SetSource(VSSource);
  shaders[vtkShader::Geometry]->SetSource(GSSource);
  shaders[vtkShader::Fragment]->SetSource(FSSource);
}

void DGState::ReplaceShaderColor(ShaderMap& shaders, vtkOpenGLCellGridRenderRequest* request)
{
  auto* mapper = request->GetMapper();

  std::string GSSource = shaders[vtkShader::Geometry]->GetSource();
  std::string FSSource = shaders[vtkShader::Fragment]->GetSource();

  // these are always defined
  std::string colorDec = "uniform float ambientIntensity; // the material ambient\n"
                         "uniform float diffuseIntensity; // the material diffuse\n"
                         "uniform float opacityUniform; // the fragment opacity\n"
                         "uniform vec3 ambientColorUniform; // ambient color\n"
                         "uniform vec3 diffuseColorUniform; // diffuse color\n";

  std::string colorImpl;

  // specular lighting?
  if (this->LastLightComplexity)
  {
    colorDec += "uniform float specularIntensity; // the material specular intensity\n"
                "uniform vec3 specularColorUniform; // intensity weighted color\n"
                "uniform float specularPowerUniform;\n";
    colorImpl += "vec3 specularColor = specularIntensity * specularColorUniform;\n"
                 "  float specularPower = specularPowerUniform;\n";
  }

  if (mapper->GetScalarVisibility() && !mapper->GetColorCoordinates())
  {
    colorDec += "uniform sampler2D colorTexture;";
    colorImpl += "  vec4 texColor = texture(colorTexture, texCoord.st);\n"
                 "  vec3 ambientColor = ambientIntensity * texColor.rgb;\n"
                 "  vec3 diffuseColor = diffuseIntensity * texColor.rgb;\n"
                 "  float opacity = opacityUniform * texColor.a;";
  }
  // just material but handle backface properties
  else
  {
    colorImpl += "  vec3 ambientColor = ambientIntensity * ambientColorUniform;\n"
                 "  vec3 diffuseColor = diffuseIntensity * diffuseColorUniform;\n"
                 "  float opacity = opacityUniform;\n";

    auto* actor = request->GetActor();
    if (actor->GetBackfaceProperty() /*&& !this->DrawingVertices*/)
    {
      colorDec += "uniform float opacityUniformBF; // the fragment opacity\n"
                  "uniform float ambientIntensityBF; // the material ambient\n"
                  "uniform float diffuseIntensityBF; // the material diffuse\n"
                  "uniform vec3 ambientColorUniformBF; // ambient material color\n"
                  "uniform vec3 diffuseColorUniformBF; // diffuse material color\n";
      if (this->LastLightComplexity)
      {
        colorDec += "uniform float specularIntensityBF; // the material specular intensity\n"
                    "uniform vec3 specularColorUniformBF; // intensity weighted color\n"
                    "uniform float specularPowerUniformBF;\n";
        colorImpl += "  if (gl_FrontFacing == false) {\n"
                     "    ambientColor = ambientIntensityBF * ambientColorUniformBF;\n"
                     "    diffuseColor = diffuseIntensityBF * diffuseColorUniformBF;\n"
                     "    specularColor = specularIntensityBF * specularColorUniformBF;\n"
                     "    specularPower = specularPowerUniformBF;\n"
                     "    opacity = opacityUniformBF; }\n";
      }
      else
      {
        colorImpl += "  if (gl_FrontFacing == false) {\n"
                     "    ambientColor = ambientIntensityBF * ambientColorUniformBF;\n"
                     "    diffuseColor = diffuseIntensityBF * diffuseColorUniformBF;\n"
                     "    opacity = opacityUniformBF; }\n";
      }
    }
  }

  vtkShaderProgram::Substitute(FSSource, "//VTK::Color::Dec", colorDec);
  vtkShaderProgram::Substitute(FSSource, "//VTK::Color::Impl", colorImpl);

  shaders[vtkShader::Geometry]->SetSource(GSSource);
  shaders[vtkShader::Fragment]->SetSource(FSSource);
}

void DGState::ReplaceShaderNormal(ShaderMap& shaders, vtkOpenGLCellGridRenderRequest* request)
{
  (void)request;
  std::string GSSource = shaders[vtkShader::Geometry]->GetSource();
  std::string FSSource = shaders[vtkShader::Fragment]->GetSource();

  vtkShaderProgram::Substitute(GSSource, "//VTK::Normal::Dec",
    "out vec3 normalVCGSOutput;"
    "uniform mat3 normalMatrix;\n");
  vtkShaderProgram::Substitute(
    GSSource, "//VTK::Normal::Impl", "normalVCGSOutput = normalMatrix * vec3(n.x, n.y, n.z);");

  vtkShaderProgram::Substitute(FSSource, "//VTK::Normal::Dec", "in vec3 normalVCGSOutput;");
  vtkShaderProgram::Substitute(FSSource, "//VTK::Normal::Impl",
    "vec3 normalVCGSOutput = normalize(normalVCGSOutput);\n"
    "  if (gl_FrontFacing == false) { normalVCGSOutput = -normalVCGSOutput; }\n");
  shaders[vtkShader::Geometry]->SetSource(GSSource);
  shaders[vtkShader::Fragment]->SetSource(FSSource);
}

void DGState::ReplaceShaderLight(ShaderMap& shaders, vtkOpenGLCellGridRenderRequest* request)
{
  std::string FSSource = shaders[vtkShader::Fragment]->GetSource();
  std::ostringstream toString;
  auto renderer = vtkOpenGLRenderer::SafeDownCast(request->GetRenderer());
  auto* mapper = request->GetMapper();

  // check for normal rendering
  auto* actor = request->GetActor();
  auto info = actor->GetPropertyKeys();
  if (info && info->Has(vtkLightingMapPass::RENDER_NORMALS()))
  {
    vtkShaderProgram::Substitute(FSSource, "//VTK::Light::Impl",
      "vec3 n = (normalVCGSOutput + 1.0f) * 0.5;\n"
      "  gl_FragData[0] = vec4(n.x, n.y, n.z, 1.0);");
    shaders[vtkShader::Fragment]->SetSource(FSSource);
    return;
  }

  // for luminance, we don't want diffuse, specular colors to show up.
  if (info && info->Has(vtkLightingMapPass::RENDER_LUMINANCE()))
  {
    vtkShaderProgram::Substitute(FSSource, "//VTK::Light::Impl",
      "diffuseColor = vec3(1.0f, 1.0f, 1.0f);\n"
      "  specularColor = vec3(1.0f, 1.0f, 1.0f);\n"
      "  //VTK::Light::Impl\n",
      false);
  }

  int lastLightComplexity = this->LastLightComplexity;
  int lastLightCount = this->LastLightCount;
  if (actor->GetProperty()->GetInterpolation() != VTK_PBR && lastLightCount == 0)
  {
    lastLightComplexity = 0;
  }

  // For now, this mapper prototype does not do image based lighting, does not consider
  // anisotropy property or clear-coating

  // get standard lighting declarations.
  vtkShaderProgram::Substitute(FSSource, "//VTK::Light::Dec", renderer->GetLightingUniforms());
  switch (lastLightComplexity)
  {
    case 0: // no lighting
      vtkShaderProgram::Substitute(FSSource, "//VTK::Light::Impl",
        "gl_FragData[0] = vec4(ambientColor + diffuseColor, opacity);\n"
        "  //VTK::Light::Impl\n",
        false);
      break;
    case 1: // headlight
      if (actor->GetProperty()->GetInterpolation() == VTK_PBR)
      {
        vtkErrorWithObjectMacro(mapper, << "Headlights are not implemented for PBR interpolation");
        break;
      }
      else
      {
        toString << "float df = max(0.0f, normalVCGSOutput.z);\n"
                    "  float sf = pow(df, specularPower);\n"
                    "  vec3 diffuse = df * diffuseColor * lightColor0;\n"
                    "  vec3 specular = sf * specularColor * lightColor0;\n"
                    "  gl_FragData[0] = vec4(ambientColor + diffuse + specular, opacity);\n"
                    "  //VTK::Light::Impl\n";
      }
      vtkShaderProgram::Substitute(FSSource, "//VTK::Light::Impl", toString.str(), false);
      break;
    case 2: // light kit
      vtkErrorWithObjectMacro(mapper, << "Light kit is not implemented!");
      break;
    case 3: // positional
      vtkErrorWithObjectMacro(mapper, << "Position lighs are not implemented!");
      break;
  }
  shaders[vtkShader::Fragment]->SetSource(FSSource);
}

void DGState::ReplaceShaderPositionVC(ShaderMap& shaders, vtkOpenGLCellGridRenderRequest* request)
{
  (void)request;
  std::string VSSource = shaders[vtkShader::Vertex]->GetSource();
  std::string GSSource = shaders[vtkShader::Geometry]->GetSource();
  std::string FSSource = shaders[vtkShader::Fragment]->GetSource();

  vtkShaderProgram::Substitute(
    FSSource, "//VTK::Camera::Dec", "uniform int cameraParallel;\n", false);

  // Do we need the vertex in the shader in View Coordinates?
  if (this->LastLightComplexity > 0 /*|| this->DrawingTubes(*this->LastBoundBO, actor)*/)
  {
    vtkShaderProgram::Substitute(GSSource, "//VTK::PositionVC::Dec", "out vec4 vertexVCGSOutput;");
    vtkShaderProgram::Substitute(GSSource, "//VTK::PositionVC::Impl",
      "vertexVCGSOutput = MCVCMatrix * vertexMC;\n"
      "        gl_Position = MCDCMatrix * vertexMC;\n");
    vtkShaderProgram::Substitute(GSSource, "//VTK::Camera::Dec",
      "uniform mat4 MCDCMatrix;\n"
      "uniform mat4 MCVCMatrix;");
    vtkShaderProgram::Substitute(GSSource, "//VTK::PositionVC::Dec",
      "in vec4 vertexVCGSOutput[];\n"
      "out vec4 vertexVCGSOutput;");
    vtkShaderProgram::Substitute(FSSource, "//VTK::PositionVC::Dec", "in vec4 vertexVCGSOutput;");
    vtkShaderProgram::Substitute(
      FSSource, "//VTK::PositionVC::Impl", "vec4 vertexVC = vertexVCGSOutput;");
  }
  else
  {
    vtkShaderProgram::Substitute(GSSource, "//VTK::Camera::Dec", "uniform mat4 MCDCMatrix;");
    vtkShaderProgram::Substitute(
      GSSource, "//VTK::PositionVC::Impl", "gl_Position = MCDCMatrix * vertexMC;\n");
  }
  shaders[vtkShader::Vertex]->SetSource(VSSource);
  shaders[vtkShader::Geometry]->SetSource(GSSource);
  shaders[vtkShader::Fragment]->SetSource(FSSource);
}

void DGState::SetMapperShaderParameters(vtkOpenGLCellGridRenderRequest* request)
{
  auto* mapper = request->GetMapper();

  // Still gotta bind the VAO; otherwise OpenGL will not render anything:
  if (this->CellBO.IBO->IndexCount /*&&
    (this->VBOs->GetMTime() > this->CellBO.AttributeUpdateTime ||
     this->CellBO.ShaderSourceTime > this->CellBO.AttributeUpdateTime ||
     this->CellBO.VAO->GetMTime() > this->CellBO.AttributeUpdateTime)*/)
  {
    this->CellBO.VAO->Bind();
    this->CellBO.AttributeUpdateTime.Modified();
  }
  vtkOpenGLStaticCheckErrorMacro("Failed after binding VAO");

  int tunit = this->PointCoordinatesTB.Texture->GetTextureUnit();
  if (!this->CellBO.Program->SetUniformi("vertexPositions", tunit))
  {
    vtkWarningWithObjectMacro(mapper, << this->CellBO.Program->GetError());
  }

  if (mapper->GetScalarVisibility())
  {
    tunit = this->FieldCoefficientsTB.Texture->GetTextureUnit();
    if (!this->CellBO.Program->SetUniformi("fieldCoefficients", tunit))
    {
      vtkWarningWithObjectMacro(mapper, << this->CellBO.Program->GetError());
    }
  }

  tunit = this->CellConnectivityTB.Texture->GetTextureUnit();
  if (!this->CellBO.Program->SetUniformi("cellConnectivity", tunit))
  {
    vtkWarningWithObjectMacro(mapper, << this->CellBO.Program->GetError());
  }

  tunit = this->SideConnectivityTB.Texture->GetTextureUnit();
  if (!this->CellBO.Program->SetUniformi("sideConnectivity", tunit))
  {
    vtkWarningWithObjectMacro(mapper, << this->CellBO.Program->GetError());
  }

  tunit = this->FaceConnectivityTB.Texture->GetTextureUnit();
  if (!this->CellBO.Program->SetUniformi("faceConnectivity", tunit))
  {
    vtkWarningWithObjectMacro(mapper, << this->CellBO.Program->GetError());
  }

  if (mapper->GetScalarVisibility())
  {
    tunit = this->CellParametricsTB.Texture->GetTextureUnit();
    if (!this->CellBO.Program->SetUniformi("cellParametrics", tunit))
    {
      vtkWarningWithObjectMacro(mapper, << this->CellBO.Program->GetError());
    }
  }

  if (mapper->GetScalarVisibility())
  {
    tunit = this->ColorTextureGL->GetTextureUnit();
    if (!this->CellBO.Program->SetUniformi("colorTexture", tunit))
    {
      vtkWarningWithObjectMacro(mapper, << this->CellBO.Program->GetError());
    }
    vtkOpenGLStaticCheckErrorMacro("failed @ color texture.");

    if (!this->CellBO.Program->SetUniformi("visualizePCoord", mapper->GetVisualizePCoords()))
    {
      vtkWarningWithObjectMacro(mapper, << this->CellBO.Program->GetError());
    }
  }
  if (!this->CellBO.Program->SetUniformi(
        "visualizeBasisFunction", mapper->GetVisualizeBasisFunction()))
  {
    vtkWarningWithObjectMacro(mapper, << this->CellBO.Program->GetError());
  }

  if (!this->CellBO.Program->SetUniformi("mapScalars", mapper->GetScalarVisibility() ? 1 : 0))
  {
    vtkWarningWithObjectMacro(mapper, << this->CellBO.Program->GetError());
  }
  if (mapper->GetScalarVisibility())
  {
    float range[2];
    auto minmax = std::minmax_element(
      this->InputFieldCoefficients->Begin(), this->InputFieldCoefficients->End());
    range[0] = *minmax.first;
    range[1] = *minmax.second;
    this->CellBO.Program->SetUniform2f("fieldRange", range);
  }

  vtkOpenGLStaticCheckErrorMacro("failed after updating shader uniforms");
}

void DGState::SetPropertyShaderParameters(vtkOpenGLCellGridRenderRequest* request)
{
  auto* actor = request->GetActor();
  vtkProperty* property = actor->GetProperty();
  vtkShaderProgram* program = this->CellBO.Program;

  // Query the property for some of the properties that can be applied.
  float opacity = property->GetOpacity();
  double* aColor = property->GetAmbientColor();
  double aIntensity = property->GetAmbient();

  double* dColor = property->GetDiffuseColor();
  double dIntensity = property->GetDiffuse();

  // double* sColor = property->GetSpecularColor();
  // double sIntensity = property->GetSpecular();
  // double specularPower = property->GetSpecularPower();

  // these are always set
  program->SetUniformf("opacityUniform", opacity);
  program->SetUniformf("ambientIntensity", aIntensity);
  program->SetUniformf("diffuseIntensity", dIntensity);
  program->SetUniform3f("ambientColorUniform", aColor);
  if (program->IsUniformUsed("diffuseColorUniform"))
  {
    program->SetUniform3f("diffuseColorUniform", dColor);
  }
}

void DGState::SetCameraShaderParameters(vtkOpenGLCellGridRenderRequest* request)
{
  auto* actor = request->GetActor();
  auto* renderer = request->GetRenderer();
  vtkShaderProgram* program = this->CellBO.Program;

  vtkOpenGLCamera* cam = (vtkOpenGLCamera*)(renderer->GetActiveCamera());

  // [WMVD]C == {world, model, view, display} coordinates
  // E.g., WCDC == world to display coordinate transformation
  vtkMatrix4x4* wcdc;
  vtkMatrix4x4* wcvc;
  vtkMatrix3x3* norms;
  vtkMatrix4x4* vcdc;
  cam->GetKeyMatrices(renderer, wcvc, norms, vcdc, wcdc);

  if (program->IsUniformUsed("ZCalcR"))
  {
    if (cam->GetParallelProjection())
    {
      program->SetUniformf("ZCalcS", vcdc->GetElement(2, 2));
    }
    else
    {
      program->SetUniformf("ZCalcS", -0.5 * vcdc->GetElement(2, 2) + 0.5);
    }
#if ENABLE_SUPPORT_SPHERE
    // if (this->DrawingSpheres(this->CellBO, actor))
    // {
    //   program->SetUniformf("ZCalcR",
    //     actor->GetProperty()->GetPointSize() / (renderer->GetSize()[0] * vcdc->GetElement(0,
    //     0)));
    // }
    // else
#endif
    {
      program->SetUniformf("ZCalcR",
        actor->GetProperty()->GetLineWidth() / (renderer->GetSize()[0] * vcdc->GetElement(0, 0)));
    }
  }

#if ENABLE_HANDLE_COINCIDENT
  // handle coincident
  float factor = 0.0;
  float offset = 0.0;
  this->GetCoincidentParameters(renderer, actor, factor, offset);
  if ((factor != 0.0 || offset != 0.0) && this->CellBO.Program->IsUniformUsed("cOffset") &&
    this->CellBO.Program->IsUniformUsed("cFactor"))
  {
    this->CellBO.Program->SetUniformf("cOffset", offset);
    this->CellBO.Program->SetUniformf("cFactor", factor);
  }
#endif

  vtkNew<vtkMatrix3x3> env;
  if (program->IsUniformUsed("envMatrix"))
  {
    double up[3];
    double right[3];
    double front[3];
    renderer->GetEnvironmentUp(up);
    renderer->GetEnvironmentRight(right);
    vtkMath::Cross(right, up, front);
    for (int i = 0; i < 3; i++)
    {
      env->SetElement(i, 0, right[i]);
      env->SetElement(i, 1, up[i]);
      env->SetElement(i, 2, front[i]);
    }
  }

  // If the VBO coordinates were shifted and scaled, apply the inverse transform
  // to the model->view matrix:
  // vtkOpenGLVertexBufferObject* vvbo = this->VBOs->GetVBO("vertexMC");
#if ENABLE_SHIFT_SCALE
  if (vvbo && vvbo->GetCoordShiftAndScaleEnabled())
  {
    if (!actor->GetIsIdentity())
    {
      vtkMatrix4x4* mcwc;
      vtkMatrix3x3* anorms;
      static_cast<vtkOpenGLActor*>(actor)->GetKeyMatrices(mcwc, anorms);
      vtkMatrix4x4::Multiply4x4(this->VBOShiftScale, mcwc, this->TempMatrix4);
      if (program->IsUniformUsed("MCWCMatrix"))
      {
        program->SetUniformMatrix("MCWCMatrix", this->TempMatrix4);
      }
      if (program->IsUniformUsed("MCWCNormalMatrix"))
      {
        program->SetUniformMatrix("MCWCNormalMatrix", anorms);
      }
      vtkMatrix4x4::Multiply4x4(this->TempMatrix4, wcdc, this->TempMatrix4);
      program->SetUniformMatrix("MCDCMatrix", this->TempMatrix4);
      if (program->IsUniformUsed("MCVCMatrix"))
      {
        vtkMatrix4x4::Multiply4x4(this->VBOShiftScale, mcwc, this->TempMatrix4);
        vtkMatrix4x4::Multiply4x4(this->TempMatrix4, wcvc, this->TempMatrix4);
        program->SetUniformMatrix("MCVCMatrix", this->TempMatrix4);
      }
      if (program->IsUniformUsed("normalMatrix"))
      {
        vtkMatrix3x3::Multiply3x3(anorms, norms, this->TempMatrix3);
        program->SetUniformMatrix("normalMatrix", this->TempMatrix3);
      }
    }
    else
    {
      vtkMatrix4x4::Multiply4x4(this->VBOShiftScale, wcdc, this->TempMatrix4);
      program->SetUniformMatrix("MCDCMatrix", this->TempMatrix4);
      if (program->IsUniformUsed("MCVCMatrix"))
      {
        vtkMatrix4x4::Multiply4x4(this->VBOShiftScale, wcvc, this->TempMatrix4);
        program->SetUniformMatrix("MCVCMatrix", this->TempMatrix4);
      }
      if (program->IsUniformUsed("normalMatrix"))
      {
        program->SetUniformMatrix("normalMatrix", norms);
      }
    }
  }
  else
#endif
  {
    if (!actor->GetIsIdentity())
    {
      vtkMatrix4x4* mcwc;
      vtkMatrix3x3* anorms;
      ((vtkOpenGLActor*)actor)->GetKeyMatrices(mcwc, anorms);
      if (program->IsUniformUsed("MCWCMatrix"))
      {
        program->SetUniformMatrix("MCWCMatrix", mcwc);
      }
      if (program->IsUniformUsed("MCWCNormalMatrix"))
      {
        program->SetUniformMatrix("MCWCNormalMatrix", anorms);
      }
      vtkMatrix4x4::Multiply4x4(mcwc, wcdc, this->TempMatrix4);
      program->SetUniformMatrix("MCDCMatrix", this->TempMatrix4);
      if (program->IsUniformUsed("MCVCMatrix"))
      {
        vtkMatrix4x4::Multiply4x4(mcwc, wcvc, this->TempMatrix4);
        program->SetUniformMatrix("MCVCMatrix", this->TempMatrix4);
      }
      if (program->IsUniformUsed("normalMatrix"))
      {
        vtkMatrix3x3::Multiply3x3(anorms, norms, this->TempMatrix3);
        program->SetUniformMatrix("normalMatrix", this->TempMatrix3);
      }
    }
    else
    {
      program->SetUniformMatrix("MCDCMatrix", wcdc);
      if (program->IsUniformUsed("MCVCMatrix"))
      {
        program->SetUniformMatrix("MCVCMatrix", wcvc);
      }
      if (program->IsUniformUsed("normalMatrix"))
      {
        program->SetUniformMatrix("normalMatrix", norms);
      }
    }
  }

  if (program->IsUniformUsed("envMatrix"))
  {
    vtkMatrix3x3::Invert(norms, this->TempMatrix3);
    vtkMatrix3x3::Multiply3x3(this->TempMatrix3, env, this->TempMatrix3);
    program->SetUniformMatrix("envMatrix", this->TempMatrix3);
  }

  if (program->IsUniformUsed("cameraParallel"))
  {
    program->SetUniformi("cameraParallel", cam->GetParallelProjection());
  }
}

void DGState::SetLightingShaderParameters(vtkOpenGLCellGridRenderRequest* request)
{
  vtkOpenGLRenderer* oglRen = vtkOpenGLRenderer::SafeDownCast(request->GetRenderer());
  if (!oglRen)
  {
    return;
  }

  vtkFloatArray* sh = oglRen->GetSphericalHarmonics();
  if (oglRen->GetUseSphericalHarmonics() && sh)
  {
    std::string uniforms[3] = { "shRed", "shGreen", "shBlue" };
    for (int i = 0; i < 3; i++)
    {
      float coeffs[9];
      sh->GetTypedTuple(i, coeffs);

      // predivide with pi for Lambertian diffuse
      coeffs[0] *= 0.282095f;
      coeffs[1] *= -0.488603f * (2.f / 3.f);
      coeffs[2] *= 0.488603f * (2.f / 3.f);
      coeffs[3] *= -0.488603f * (2.f / 3.f);
      coeffs[4] *= 1.092548f * 0.25f;
      coeffs[5] *= -1.092548f * 0.25f;
      coeffs[6] *= 0.315392f * 0.25f;
      coeffs[7] *= -1.092548f * 0.25f;
      coeffs[8] *= 0.546274f * 0.25f;

      this->CellBO.Program->SetUniform1fv(uniforms[i].c_str(), 9, coeffs);
    }
  }
  oglRen->UpdateLightingUniforms(this->CellBO.Program);
}

bool vtkDGOpenGLRenderer::Query(vtkOpenGLCellGridRenderRequest* request,
  vtkCellMetadata* cellMetadata, vtkCellGridResponders* caches)
{
  (void)caches;

  if (!request->GetIsReleasingResources())
  {
    return this->DrawCells(request, cellMetadata);
  }

  return this->ReleaseResources(request, cellMetadata);
}

bool vtkDGOpenGLRenderer::DrawCells(
  vtkOpenGLCellGridRenderRequest* request, vtkCellMetadata* cellMetadata)
{
  bool didDraw = false;
  auto* dgCellType = dynamic_cast<vtkDGCell*>(cellMetadata);
  if (!dgCellType)
  {
    return didDraw;
  }

  int cellDim = dgCellType->GetDimension();
  int shapes = request->GetShapesToDraw();
  // If we can render the cell itself as a primitive (i.e., it has dimension < 3),
  // and it is requested, then draw it.
  if (cellDim < 3 && ((1 << cellDim) & shapes))
  {
    didDraw |= this->DrawShapes(request, dgCellType, -1);
  }

  // Now if any side arrays are present, draw those which match the request.
  int numberOfSideTypes = dgCellType->GetNumberOfSideTypes();
  for (int tt = 0; tt < numberOfSideTypes; ++tt)
  {
    auto range = dgCellType->GetSideRangeForType(tt);
    auto shape = dgCellType->GetSideShape(range.first);
    int shapeDim = vtkDGCell::GetShapeDimension(shape);
    // std::cout << "Foo " << (1 << shapeDim) << " vs " << shapes << ".\n";
    if (((1 << shapeDim) & shapes) == 0)
    {
      vtkDebugMacro(
        "Skipping " << vtkDGCell::GetShapeName(shape).Data() << " sides; shape bit unset.");
      // The shapes flag says to skip these sides.
      continue;
    }
    didDraw |= this->DrawShapes(request, dgCellType, shape);
  }
  return didDraw;
}

bool vtkDGOpenGLRenderer::DrawShapes(
  vtkOpenGLCellGridRenderRequest* request, vtkDGCell* cellType, int shape)
{
  // Fetch the cell-grid containing arrays defining the cell.
  auto* grid = cellType->GetCellGrid();
  if (!grid)
  {
    vtkErrorWithObjectMacro(cellType, << "Cell metadata has no parent cell-grid.");
    return false;
  }

  std::string shortCellType = cellType->GetClassName();
  shortCellType = shortCellType.substr(3); // Trim leading "vtk"
  vtkStringToken shortCellToken(shortCellType);

  std::string sideAttributeName;
  if (shape == -1)
  {
    sideAttributeName = shortCellType;
  }
  else
  {
    std::ostringstream name;
    name << vtkDGCell::GetShapeName(static_cast<vtkDGCell::Shape>(shape)).Data() << " sides of "
         << shortCellType;
    sideAttributeName = name.str();
  }

  auto* inputSides = grid->GetAttributes(sideAttributeName)->GetArray("conn");
  if (!inputSides)
  {
    // No such sides exist.
    return false;
  }

  // I. Populate inputs
  // Fetch renderer state (texture objects, buffer objects, array objects, etc.)
  auto* state = request->GetState<DGState>(sideAttributeName);
  if (!state)
  {
    return false;
  }
  state->SetMetadataAndSideShape(cellType, shape, shortCellToken);

  state->InputPoints->Reset();
  state->InputCells->Reset();

  auto renderer = request->GetRenderer();
  auto oglRenWin = vtkOpenGLRenderWindow::SafeDownCast(renderer->GetRenderWindow());

  state->InputPoints->ShallowCopy(grid->GetAttributes("coordinates"_token)->GetVectors());
  state->InputCells->ShallowCopy(grid->GetAttributes(shortCellToken)->GetArray("conn"));
  state->InputSides->ShallowCopy(grid->GetAttributes(sideAttributeName)->GetArray("conn"));
  // state->ParametricCoordinates is initialized in SetMetadataAndSideShape.
  // state->FaceConnectivity is initialized in SetMetadataAndSideShape.

  // If coloring by an array, set it up.
  bool haveColorArray = false;
  if (request->GetMapper()->GetScalarVisibility())
  {
    // Fetch the cell-grid attribute defining the color-by scalar:
    auto* cellAttribute = grid->GetCellAttributeByName(request->GetMapper()->GetArrayName());

    // Fetch the array of coefficients used to perform scalar interpolation before colormap lookup:
    // Because this code is specific to the cell type, it knows which array(s) to fetch from the
    // cellAttribute.
    auto arraysForCellType = cellAttribute->GetArraysForCellType(state->CellTypeToken);
    auto* scalars = vtkDataArray::SafeDownCast(arraysForCellType[state->ShortTypeToken]);

    // Another way to fetch the array for our test dataset:
    // auto* scalars =
    // grid->GetAttributes(state->ShortTypeToken)->GetArray(request->GetMapper()->GetArrayName());

    state->InputFieldCoefficients->ShallowCopy(scalars);
    // request->GetMapper()->MapScalars(1.0); // This constructs an array we don't need/want.
    haveColorArray = true;
  }
  else
  {
    state->InputFieldCoefficients->Reset();
  }

  // II. Render start
  //     Update buffer objects
  //     a. IBO
  state->CellBO.IBO->IndexCount = state->InputSides->GetNumberOfTuples();
  std::vector<unsigned int> sideIndices;
  sideIndices.resize(state->CellBO.IBO->IndexCount);
  std::iota(sideIndices.begin(), sideIndices.end(), 0);
  state->CellBO.IBO->Upload(sideIndices, vtkOpenGLBufferObject::ElementArrayBuffer);

  //     b. VBO (nothing yet)

  //     c. TBOs
  //        i. Cell connectivity
  state->CellConnectivityTB.Buffer->SetType(vtkOpenGLBufferObject::TextureBuffer);
  state->CellConnectivityTB.Texture->SetContext(oglRenWin);
  state->CellConnectivityTB.Buffer->Upload(state->InputCells->GetPointer(0),
    state->InputCells->GetNumberOfValues(), vtkOpenGLBufferObject::TextureBuffer);
  state->CellConnectivityTB.Texture->SetRequireTextureInteger(true);
  state->CellConnectivityTB.Texture->GetInternalFormat(VTK_INT, 1, true);
  state->CellConnectivityTB.Texture->CreateTextureBuffer(state->InputCells->GetNumberOfValues(), 1,
    state->InputCells->GetDataType(), state->CellConnectivityTB.Buffer);
  vtkOpenGLStaticCheckErrorMacro("Failed to upload cell connectivity.");

  //        ii. Side IDs
  state->SideConnectivityTB.Buffer->SetType(vtkOpenGLBufferObject::TextureBuffer);
  state->SideConnectivityTB.Texture->SetContext(oglRenWin);
  state->SideConnectivityTB.Buffer->Upload(state->InputSides->GetPointer(0),
    state->InputSides->GetNumberOfValues(), vtkOpenGLBufferObject::TextureBuffer);
  state->SideConnectivityTB.Texture->SetRequireTextureInteger(true);
  state->SideConnectivityTB.Texture->GetInternalFormat(VTK_INT, 1, true);
  state->SideConnectivityTB.Texture->CreateTextureBuffer(state->InputSides->GetNumberOfValues(), 1,
    state->InputSides->GetDataType(), state->SideConnectivityTB.Buffer);
  vtkOpenGLStaticCheckErrorMacro("Failed to upload side connectivity.");

  //        iii. Side connectivity.
  state->FaceConnectivityTB.Buffer->SetType(vtkOpenGLBufferObject::TextureBuffer);
  state->FaceConnectivityTB.Texture->SetContext(oglRenWin);
  state->FaceConnectivityTB.Buffer->Upload(state->FaceConnectivity->GetPointer(0),
    state->FaceConnectivity->GetNumberOfValues(), vtkOpenGLBufferObject::TextureBuffer);
  state->FaceConnectivityTB.Texture->SetRequireTextureInteger(true);
  state->FaceConnectivityTB.Texture->GetInternalFormat(VTK_INT, 1, true);
  state->FaceConnectivityTB.Texture->CreateTextureBuffer(
    state->FaceConnectivity->GetNumberOfValues(), 1, state->FaceConnectivity->GetDataType(),
    state->FaceConnectivityTB.Buffer);
  vtkOpenGLStaticCheckErrorMacro("Failed to upload cell-side connectivity.");

  //        iv. Parametric coordinates of cell corners
  state->CellParametricsTB.Buffer->SetType(vtkOpenGLBufferObject::TextureBuffer);
  state->CellParametricsTB.Texture->SetContext(oglRenWin);
  state->CellParametricsTB.Buffer->Upload(state->ParametricCoordinates->GetPointer(0),
    state->ParametricCoordinates->GetNumberOfValues(), vtkOpenGLBufferObject::TextureBuffer);
  state->CellParametricsTB.Texture->CreateTextureBuffer(
    8, 3, VTK_FLOAT, state->CellParametricsTB.Buffer);
  vtkOpenGLStaticCheckErrorMacro("Failed to upload corner-parameter texture.");

  //        v. Point Coordinates
  state->PointCoordinatesTB.Buffer->SetType(vtkOpenGLBufferObject::TextureBuffer);
  state->PointCoordinatesTB.Texture->SetContext(oglRenWin);
  state->PointCoordinatesTB.Buffer->Upload(state->InputPoints->GetPointer(0),
    state->InputPoints->GetNumberOfValues(), vtkOpenGLBufferObject::TextureBuffer);
  assert(state->InputPoints->GetNumberOfComponents() == 3);
  state->PointCoordinatesTB.Texture->CreateTextureBuffer(state->InputPoints->GetNumberOfTuples(), 3,
    state->InputPoints->GetDataType(), state->PointCoordinatesTB.Buffer);
  vtkOpenGLStaticCheckErrorMacro("Failed to upload point-coordinates texture");

  //        vi. Field-value array to color by
  if (haveColorArray)
  {
    state->FieldCoefficientsTB.Buffer->SetType(vtkOpenGLBufferObject::TextureBuffer);
    state->FieldCoefficientsTB.Texture->SetContext(oglRenWin);
    state->FieldCoefficientsTB.Buffer->Upload(state->InputFieldCoefficients->GetPointer(0),
      state->InputFieldCoefficients->GetNumberOfValues(), vtkOpenGLBufferObject::TextureBuffer);
    state->FieldCoefficientsTB.Texture->CreateTextureBuffer(
      state->InputFieldCoefficients->GetNumberOfValues(), 1,
      state->InputFieldCoefficients->GetDataType(), state->FieldCoefficientsTB.Buffer);
    vtkOpenGLStaticCheckErrorMacro("Failed to upload scalar color texture.");

    // If we are coloring by texture, then load the texture map.
    state->ColorTextureGL->RepeatOff();
    state->ColorTextureGL->SetInputData(request->GetMapper()->GetColorTextureMap());
  }

  // Activate the objects.
  state->CellConnectivityTB.Texture->Activate();
  state->SideConnectivityTB.Texture->Activate();
  state->FaceConnectivityTB.Texture->Activate();
  state->CellParametricsTB.Texture->Activate();
  state->PointCoordinatesTB.Texture->Activate();
  if (haveColorArray)
  {
    state->FieldCoefficientsTB.Texture->Activate();
    state->ColorTextureGL->Load(renderer);
  }

  // III. Render draw
  if (renderer->GetUseImageBasedLighting() && renderer->GetEnvironmentTexture())
  {
    vtkOpenGLState* ostate = oglRenWin->GetState();
    ostate->vtkglEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
  }
  //      a. Update shaders
  state->CellBO.VAO->Bind();
  // state->LastBoundBO = &state->CellBO; // We only bind the one...
  state->RebuildShadersIfNeeded(request);
  if (state->CellBO.Program)
  {
    state->SetMapperShaderParameters(request);
    state->SetPropertyShaderParameters(request);
    state->SetCameraShaderParameters(request);
    state->SetLightingShaderParameters(request);
    request->GetMapper()->InvokeEvent(vtkCommand::UpdateShaderEvent, state->CellBO.Program);
  }

  //      b. Draw elements
  state->CellBO.IBO->Bind();
  glDrawRangeElements(GL_POINTS, 0, static_cast<GLuint>(state->CellBO.IBO->IndexCount - 1),
    static_cast<GLsizei>(state->CellBO.IBO->IndexCount), GL_UNSIGNED_INT, nullptr);
  vtkOpenGLStaticCheckErrorMacro("Failure after glDrawRangeElements.");
  state->CellBO.IBO->Release();

  // IV. Render finish
  //     a. Release last bound BO
  state->CellBO.VAO->Release();

  //     b. Deactivate TBOs
  state->CellConnectivityTB.Texture->Deactivate();
  state->SideConnectivityTB.Texture->Deactivate();
  state->FaceConnectivityTB.Texture->Deactivate();
  state->CellParametricsTB.Texture->Deactivate();
  state->PointCoordinatesTB.Texture->Deactivate();
  if (haveColorArray)
  {
    state->FieldCoefficientsTB.Texture->Deactivate();
    state->ColorTextureGL->PostRender(renderer);
  }

  return true;
}

bool vtkDGOpenGLRenderer::ReleaseResources(
  vtkOpenGLCellGridRenderRequest* request, vtkCellMetadata* metadata)
{
  auto* state = request->GetState<DGState>(metadata->GetClassName());
  auto* window = request->GetWindow();
  if (!state || !window)
  {
    return false;
  }

  state->CellBO.ReleaseGraphicsResources(window);
  state->CellConnectivityTB.Texture->ReleaseGraphicsResources(window);
  state->SideConnectivityTB.Texture->ReleaseGraphicsResources(window);
  state->FaceConnectivityTB.Texture->ReleaseGraphicsResources(window);
  state->CellParametricsTB.Texture->ReleaseGraphicsResources(window);
  state->PointCoordinatesTB.Texture->ReleaseGraphicsResources(window);
  state->FieldCoefficientsTB.Texture->ReleaseGraphicsResources(window);
  state->ColorTextureGL->ReleaseGraphicsResources(window);
  // this->Modified(); // Needed? We don't currently test timestamps on it.

  return true;
}

VTK_ABI_NAMESPACE_END
