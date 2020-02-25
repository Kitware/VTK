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

#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkFloatArray.h"
#include "vtkHardwareSelector.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkLightingMapPass.h"
#include "vtkMath.h"
#include "vtkMatrix3x3.h"
#include "vtkMatrix4x4.h"

#include "vtkObjectFactory.h"
#include "vtkOpenGLActor.h"
#include "vtkOpenGLBufferObject.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLCellToVTKCellMap.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLHelper.h"
#include "vtkOpenGLIndexBufferObject.h"
#include "vtkOpenGLRenderPass.h"
#include "vtkOpenGLRenderTimer.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLResourceFreeCallback.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLShaderProperty.h"
#include "vtkOpenGLState.h"
#include "vtkOpenGLTexture.h"
#include "vtkOpenGLUniforms.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkOpenGLVertexBufferObject.h"
#include "vtkOpenGLVertexBufferObjectCache.h"
#include "vtkOpenGLVertexBufferObjectGroup.h"
#include "vtkPBRIrradianceTexture.h"
#include "vtkPBRLUTTexture.h"
#include "vtkPBRPrefilterTexture.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkScalarsToColors.h"
#include "vtkShaderProgram.h"
#include "vtkTextureObject.h"
#include "vtkTransform.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"

// Bring in our fragment lit shader symbols.
#include "vtkPolyDataFS.h"
#include "vtkPolyDataVS.h"
#include "vtkPolyDataWideLineGS.h"

#include <algorithm>
#include <sstream>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLPolyDataMapper);

//-----------------------------------------------------------------------------
vtkOpenGLPolyDataMapper::vtkOpenGLPolyDataMapper()
  : UsingScalarColoring(false)
  , TimerQuery(new vtkOpenGLRenderTimer)
{
  this->InternalColorTexture = nullptr;
  this->PopulateSelectionSettings = 1;
  this->LastSelectionState = vtkHardwareSelector::MIN_KNOWN_PASS - 1;
  this->CurrentInput = nullptr;
  this->TempMatrix4 = vtkMatrix4x4::New();
  this->TempMatrix3 = vtkMatrix3x3::New();
  this->DrawingEdgesOrVertices = false;
  this->ForceTextureCoordinates = false;

  this->PrimitiveIDOffset = 0;
  this->ShiftScaleMethod = vtkOpenGLVertexBufferObject::AUTO_SHIFT_SCALE;

  this->CellScalarTexture = nullptr;
  this->CellScalarBuffer = nullptr;
  this->CellNormalTexture = nullptr;
  this->CellNormalBuffer = nullptr;

  this->HaveCellScalars = false;
  this->HaveCellNormals = false;

  this->PointIdArrayName = nullptr;
  this->CellIdArrayName = nullptr;
  this->ProcessIdArrayName = nullptr;
  this->CompositeIdArrayName = nullptr;
  this->VBOs = vtkOpenGLVertexBufferObjectGroup::New();

  this->LastBoundBO = nullptr;

  for (int i = PrimitiveStart; i < PrimitiveEnd; i++)
  {
    this->LastLightComplexity[&this->Primitives[i]] = -1;
    this->LastLightCount[&this->Primitives[i]] = 0;
    this->Primitives[i].PrimitiveType = i;
  }

  this->ResourceCallback = new vtkOpenGLResourceFreeCallback<vtkOpenGLPolyDataMapper>(
    this, &vtkOpenGLPolyDataMapper::ReleaseGraphicsResources);

  // initialize to 1 as 0 indicates we have initiated a request
  this->TimerQueryCounter = 1;
  this->TimeToDraw = 0.0001;
}

//-----------------------------------------------------------------------------
vtkOpenGLPolyDataMapper::~vtkOpenGLPolyDataMapper()
{
  if (this->ResourceCallback)
  {
    this->ResourceCallback->Release();
    delete this->ResourceCallback;
    this->ResourceCallback = nullptr;
  }
  if (this->InternalColorTexture)
  { // Resources released previously.
    this->InternalColorTexture->Delete();
    this->InternalColorTexture = nullptr;
  }
  this->TempMatrix3->Delete();
  this->TempMatrix4->Delete();

  if (this->CellScalarTexture)
  { // Resources released previously.
    this->CellScalarTexture->Delete();
    this->CellScalarTexture = nullptr;
  }
  if (this->CellScalarBuffer)
  { // Resources released previously.
    this->CellScalarBuffer->Delete();
    this->CellScalarBuffer = nullptr;
  }

  if (this->CellNormalTexture)
  { // Resources released previously.
    this->CellNormalTexture->Delete();
    this->CellNormalTexture = nullptr;
  }
  if (this->CellNormalBuffer)
  { // Resources released previously.
    this->CellNormalBuffer->Delete();
    this->CellNormalBuffer = nullptr;
  }

  this->SetPointIdArrayName(nullptr);
  this->SetCellIdArrayName(nullptr);
  this->SetProcessIdArrayName(nullptr);
  this->SetCompositeIdArrayName(nullptr);
  this->VBOs->Delete();
  this->VBOs = nullptr;

  delete TimerQuery;
}

//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::ReleaseGraphicsResources(vtkWindow* win)
{
  if (!this->ResourceCallback->IsReleasing())
  {
    this->ResourceCallback->Release();
    return;
  }

  this->VBOs->ReleaseGraphicsResources(win);
  for (int i = PrimitiveStart; i < PrimitiveEnd; i++)
  {
    this->Primitives[i].ReleaseGraphicsResources(win);
  }

  if (this->InternalColorTexture)
  {
    this->InternalColorTexture->ReleaseGraphicsResources(win);
  }
  if (this->CellScalarTexture)
  {
    this->CellScalarTexture->ReleaseGraphicsResources(win);
  }
  if (this->CellScalarBuffer)
  {
    this->CellScalarBuffer->ReleaseGraphicsResources();
  }
  if (this->CellNormalTexture)
  {
    this->CellNormalTexture->ReleaseGraphicsResources(win);
  }
  if (this->CellNormalBuffer)
  {
    this->CellNormalBuffer->ReleaseGraphicsResources();
  }
  this->TimerQuery->ReleaseGraphicsResources();
  this->VBOBuildState.Clear();
  this->IBOBuildState.Clear();
  this->CellTextureBuildState.Clear();
  this->Modified();
}

#ifndef VTK_LEGACY_REMOVE

//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::AddShaderReplacement(
  vtkShader::Type shaderType, // vertex, fragment, etc
  const std::string& originalValue,
  bool replaceFirst, // do this replacement before the default
  const std::string& replacementValue, bool replaceAll)
{
  VTK_LEGACY_REPLACED_BODY(vtkOpenGLPolyDataMapper::AddShaderReplacement, "VTK 9.0",
    vtkOpenGLShaderProperty::AddShaderReplacement);
  this->GetLegacyShaderProperty()->AddShaderReplacement(
    shaderType, originalValue, replaceFirst, replacementValue, replaceAll);
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::ClearShaderReplacement(
  vtkShader::Type shaderType, // vertex, fragment, etc
  const std::string& originalValue, bool replaceFirst)
{
  VTK_LEGACY_REPLACED_BODY(vtkOpenGLPolyDataMapper::ClearShaderReplacement, "VTK 9.0",
    vtkOpenGLShaderProperty::ClearShaderReplacement);
  this->GetLegacyShaderProperty()->ClearShaderReplacement(shaderType, originalValue, replaceFirst);
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::ClearAllShaderReplacements(vtkShader::Type shaderType)
{
  VTK_LEGACY_REPLACED_BODY(vtkOpenGLPolyDataMapper::ClearAllShaderReplacements, "VTK 9.0",
    vtkOpenGLShaderProperty::ClearAllShaderReplacements);
  this->GetLegacyShaderProperty()->ClearAllShaderReplacements(shaderType);
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::ClearAllShaderReplacements()
{
  this->GetLegacyShaderProperty()->ClearAllShaderReplacements();
  this->Modified();
}

void vtkOpenGLPolyDataMapper::SetVertexShaderCode(const char* code)
{
  VTK_LEGACY_REPLACED_BODY(vtkOpenGLPolyDataMapper::SetVertexShaderCode, "VTK 9.0",
    vtkOpenGLShaderProperty::SetVertexShaderCode);
  this->GetLegacyShaderProperty()->SetVertexShaderCode(code);
  this->Modified();
}

char* vtkOpenGLPolyDataMapper::GetVertexShaderCode()
{
  VTK_LEGACY_REPLACED_BODY(vtkOpenGLPolyDataMapper::GetVertexShaderCode, "VTK 9.0",
    vtkOpenGLShaderProperty::GetVertexShaderCode);
  return this->GetLegacyShaderProperty()->GetVertexShaderCode();
}

void vtkOpenGLPolyDataMapper::SetFragmentShaderCode(const char* code)
{
  VTK_LEGACY_REPLACED_BODY(vtkOpenGLPolyDataMapper::SetFragmentShaderCode, "VTK 9.0",
    vtkOpenGLShaderProperty::SetFragmentShaderCode);
  this->GetLegacyShaderProperty()->SetFragmentShaderCode(code);
  this->Modified();
}

char* vtkOpenGLPolyDataMapper::GetFragmentShaderCode()
{
  VTK_LEGACY_REPLACED_BODY(vtkOpenGLPolyDataMapper::GetFragmentShaderCode, "VTK 9.0",
    vtkOpenGLShaderProperty::GetFragmentShaderCode);
  return this->GetLegacyShaderProperty()->GetFragmentShaderCode();
}

void vtkOpenGLPolyDataMapper::SetGeometryShaderCode(const char* code)
{
  VTK_LEGACY_REPLACED_BODY(vtkOpenGLPolyDataMapper::SetGeometryShaderCode, "VTK 9.0",
    vtkOpenGLShaderProperty::SetGeometryShaderCode);
  this->GetLegacyShaderProperty()->SetGeometryShaderCode(code);
  this->Modified();
}

char* vtkOpenGLPolyDataMapper::GetGeometryShaderCode()
{
  VTK_LEGACY_REPLACED_BODY(vtkOpenGLPolyDataMapper::GetGeometryShaderCode, "VTK 9.0",
    vtkOpenGLShaderProperty::GetGeometryShaderCode);
  return this->GetLegacyShaderProperty()->GetGeometryShaderCode();
}

// Create the shader property if it doesn't exist
vtkOpenGLShaderProperty* vtkOpenGLPolyDataMapper::GetLegacyShaderProperty()
{
  if (!this->LegacyShaderProperty)
    this->LegacyShaderProperty = vtkSmartPointer<vtkOpenGLShaderProperty>::New();
  return this->LegacyShaderProperty;
}
#endif

//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::BuildShaders(
  std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* actor)
{
#ifndef VTK_LEGACY_REMOVE
  // in cases where LegacyShaderProperty is not nullptr, it means someone has used
  // legacy shader replacement functions, so we make sure the actor uses the same
  // shader property. NOTE: this implies that it is not possible to use both legacy
  // and new functionality on the same actor/mapper.
  if (this->LegacyShaderProperty && actor->GetShaderProperty() != this->LegacyShaderProperty)
  {
    actor->SetShaderProperty(this->LegacyShaderProperty);
  }
#endif

  this->GetShaderTemplate(shaders, ren, actor);

  // user specified pre replacements
  vtkOpenGLShaderProperty* sp = vtkOpenGLShaderProperty::SafeDownCast(actor->GetShaderProperty());
  vtkOpenGLShaderProperty::ReplacementMap repMap = sp->GetAllShaderReplacements();
  for (auto i : repMap)
  {
    if (i.first.ReplaceFirst)
    {
      std::string ssrc = shaders[i.first.ShaderType]->GetSource();
      vtkShaderProgram::Substitute(
        ssrc, i.first.OriginalValue, i.second.Replacement, i.second.ReplaceAll);
      shaders[i.first.ShaderType]->SetSource(ssrc);
    }
  }

  this->ReplaceShaderValues(shaders, ren, actor);

  // user specified post replacements
  for (auto i : repMap)
  {
    if (!i.first.ReplaceFirst)
    {
      std::string ssrc = shaders[i.first.ShaderType]->GetSource();
      vtkShaderProgram::Substitute(
        ssrc, i.first.OriginalValue, i.second.Replacement, i.second.ReplaceAll);
      shaders[i.first.ShaderType]->SetSource(ssrc);
    }
  }
}

//-----------------------------------------------------------------------------
bool vtkOpenGLPolyDataMapper::HaveWideLines(vtkRenderer* ren, vtkActor* actor)
{
  if (this->GetOpenGLMode(
        actor->GetProperty()->GetRepresentation(), this->LastBoundBO->PrimitiveType) == GL_LINES &&
    actor->GetProperty()->GetLineWidth() > 1.0)
  {
    // we have wide lines, but the OpenGL implementation may
    // actually support them, check the range to see if we
    // really need have to implement our own wide lines
    vtkOpenGLRenderWindow* renWin = vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());
    return actor->GetProperty()->GetRenderLinesAsTubes() ||
      !(renWin && renWin->GetMaximumHardwareLineWidth() >= actor->GetProperty()->GetLineWidth());
  }
  return false;
}

//-----------------------------------------------------------------------------
vtkMTimeType vtkOpenGLPolyDataMapper::GetRenderPassStageMTime(vtkActor* actor)
{
  vtkInformation* info = actor->GetPropertyKeys();
  vtkMTimeType renderPassMTime = 0;

  int curRenderPasses = 0;
  if (info && info->Has(vtkOpenGLRenderPass::RenderPasses()))
  {
    curRenderPasses = info->Length(vtkOpenGLRenderPass::RenderPasses());
  }

  int lastRenderPasses = 0;
  if (this->LastRenderPassInfo->Has(vtkOpenGLRenderPass::RenderPasses()))
  {
    lastRenderPasses = this->LastRenderPassInfo->Length(vtkOpenGLRenderPass::RenderPasses());
  }
  else // have no last pass
  {
    if (!info) // have no current pass
    {
      return 0; // short circuit
    }
  }

  // Determine the last time a render pass changed stages:
  if (curRenderPasses != lastRenderPasses)
  {
    // Number of passes changed, definitely need to update.
    // Fake the time to force an update:
    renderPassMTime = VTK_MTIME_MAX;
  }
  else
  {
    // Compare the current to the previous render passes:
    for (int i = 0; i < curRenderPasses; ++i)
    {
      vtkObjectBase* curRP = info->Get(vtkOpenGLRenderPass::RenderPasses(), i);
      vtkObjectBase* lastRP = this->LastRenderPassInfo->Get(vtkOpenGLRenderPass::RenderPasses(), i);

      if (curRP != lastRP)
      {
        // Render passes have changed. Force update:
        renderPassMTime = VTK_MTIME_MAX;
        break;
      }
      else
      {
        // Render passes have not changed -- check MTime.
        vtkOpenGLRenderPass* rp = static_cast<vtkOpenGLRenderPass*>(curRP);
        renderPassMTime = std::max(renderPassMTime, rp->GetShaderStageMTime());
      }
    }
  }

  // Cache the current set of render passes for next time:
  if (info)
  {
    this->LastRenderPassInfo->CopyEntry(info, vtkOpenGLRenderPass::RenderPasses());
  }
  else
  {
    this->LastRenderPassInfo->Clear();
  }

  return renderPassMTime;
}

std::string vtkOpenGLPolyDataMapper::GetTextureCoordinateName(const char* tname)
{
  for (auto it : this->ExtraAttributes)
  {
    if (it.second.TextureName == tname)
    {
      return it.first;
    }
  }
  return std::string("tcoord");
}

//-----------------------------------------------------------------------------
bool vtkOpenGLPolyDataMapper::HaveTextures(vtkActor* actor)
{
  return (this->GetNumberOfTextures(actor) > 0);
}

typedef std::pair<vtkTexture*, std::string> texinfo;

//-----------------------------------------------------------------------------
unsigned int vtkOpenGLPolyDataMapper::GetNumberOfTextures(vtkActor* actor)
{
  unsigned int res = 0;
  if (this->ColorTextureMap)
  {
    res++;
  }
  if (actor->GetTexture())
  {
    res++;
  }
  res += actor->GetProperty()->GetNumberOfTextures();
  return res;
}

//-----------------------------------------------------------------------------
std::vector<texinfo> vtkOpenGLPolyDataMapper::GetTextures(vtkActor* actor)
{
  std::vector<texinfo> res;

  if (this->ColorTextureMap)
  {
    res.push_back(texinfo(this->InternalColorTexture, "colortexture"));
  }
  if (actor->GetTexture())
  {
    res.push_back(texinfo(actor->GetTexture(), "actortexture"));
  }
  auto textures = actor->GetProperty()->GetAllTextures();
  for (auto ti : textures)
  {
    res.push_back(texinfo(ti.second, ti.first));
  }
  return res;
}

//-----------------------------------------------------------------------------
bool vtkOpenGLPolyDataMapper::HaveTCoords(vtkPolyData* poly)
{
  return (
    this->ColorCoordinates || poly->GetPointData()->GetTCoords() || this->ForceTextureCoordinates);
}

//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::GetShaderTemplate(
  std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* actor)
{
  vtkShaderProperty* sp = actor->GetShaderProperty();
  if (sp->HasVertexShaderCode())
  {
    shaders[vtkShader::Vertex]->SetSource(sp->GetVertexShaderCode());
  }
  else
  {
    shaders[vtkShader::Vertex]->SetSource(vtkPolyDataVS);
  }

  if (sp->HasFragmentShaderCode())
  {
    shaders[vtkShader::Fragment]->SetSource(sp->GetFragmentShaderCode());
  }
  else
  {
    shaders[vtkShader::Fragment]->SetSource(vtkPolyDataFS);
  }

  if (sp->HasGeometryShaderCode())
  {
    shaders[vtkShader::Geometry]->SetSource(sp->GetGeometryShaderCode());
  }
  else
  {
    if (this->HaveWideLines(ren, actor))
    {
      shaders[vtkShader::Geometry]->SetSource(vtkPolyDataWideLineGS);
    }
    else
    {
      shaders[vtkShader::Geometry]->SetSource("");
    }
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::ReplaceShaderRenderPass(
  std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer*, vtkActor* act, bool prePass)
{
  std::string VSSource = shaders[vtkShader::Vertex]->GetSource();
  std::string GSSource = shaders[vtkShader::Geometry]->GetSource();
  std::string FSSource = shaders[vtkShader::Fragment]->GetSource();

  vtkInformation* info = act->GetPropertyKeys();
  if (info && info->Has(vtkOpenGLRenderPass::RenderPasses()))
  {
    int numRenderPasses = info->Length(vtkOpenGLRenderPass::RenderPasses());
    for (int i = 0; i < numRenderPasses; ++i)
    {
      vtkObjectBase* rpBase = info->Get(vtkOpenGLRenderPass::RenderPasses(), i);
      vtkOpenGLRenderPass* rp = static_cast<vtkOpenGLRenderPass*>(rpBase);
      if (prePass)
      {
        if (!rp->PreReplaceShaderValues(VSSource, GSSource, FSSource, this, act))
        {
          vtkErrorMacro(
            "vtkOpenGLRenderPass::ReplaceShaderValues failed for " << rp->GetClassName());
        }
      }
      else
      {
        if (!rp->PostReplaceShaderValues(VSSource, GSSource, FSSource, this, act))
        {
          vtkErrorMacro(
            "vtkOpenGLRenderPass::ReplaceShaderValues failed for " << rp->GetClassName());
        }
      }
    }
  }

  shaders[vtkShader::Vertex]->SetSource(VSSource);
  shaders[vtkShader::Geometry]->SetSource(GSSource);
  shaders[vtkShader::Fragment]->SetSource(FSSource);
}

//------------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::ReplaceShaderCustomUniforms(
  std::map<vtkShader::Type, vtkShader*> shaders, vtkActor* actor)
{
  vtkShaderProperty* sp = actor->GetShaderProperty();

  vtkShader* vertexShader = shaders[vtkShader::Vertex];
  vtkOpenGLUniforms* vu = static_cast<vtkOpenGLUniforms*>(sp->GetVertexCustomUniforms());
  vtkShaderProgram::Substitute(vertexShader, "//VTK::CustomUniforms::Dec", vu->GetDeclarations());

  vtkShader* fragmentShader = shaders[vtkShader::Fragment];
  vtkOpenGLUniforms* fu = static_cast<vtkOpenGLUniforms*>(sp->GetFragmentCustomUniforms());
  vtkShaderProgram::Substitute(fragmentShader, "//VTK::CustomUniforms::Dec", fu->GetDeclarations());

  vtkShader* geometryShader = shaders[vtkShader::Geometry];
  vtkOpenGLUniforms* gu = static_cast<vtkOpenGLUniforms*>(sp->GetGeometryCustomUniforms());
  vtkShaderProgram::Substitute(geometryShader, "//VTK::CustomUniforms::Dec", gu->GetDeclarations());
}

//------------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::ReplaceShaderColor(
  std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* actor)
{
  std::string VSSource = shaders[vtkShader::Vertex]->GetSource();
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
  if (this->LastLightComplexity[this->LastBoundBO])
  {
    colorDec += "uniform float specularIntensity; // the material specular intensity\n"
                "uniform vec3 specularColorUniform; // intensity weighted color\n"
                "uniform float specularPowerUniform;\n";
    colorImpl += "  vec3 specularColor = specularIntensity * specularColorUniform;\n"
                 "  float specularPower = specularPowerUniform;\n";
  }

  // for point picking we render primitives as points
  // that means cell scalars will not have correct
  // primitiveIds to lookup into the texture map
  // so we must skip cell scalar coloring when point picking
  // The boolean will be used in an else clause below
  vtkHardwareSelector* selector = ren->GetSelector();
  bool pointPicking = false;
  if (selector && selector->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    pointPicking = true;
  }

  // handle color point attributes
  if (this->VBOs->GetNumberOfComponents("scalarColor") != 0 && !this->DrawingEdgesOrVertices)
  {
    vtkShaderProgram::Substitute(VSSource, "//VTK::Color::Dec",
      "in vec4 scalarColor;\n"
      "out vec4 vertexColorVSOutput;");
    vtkShaderProgram::Substitute(
      VSSource, "//VTK::Color::Impl", "vertexColorVSOutput = scalarColor;");
    vtkShaderProgram::Substitute(GSSource, "//VTK::Color::Dec",
      "in vec4 vertexColorVSOutput[];\n"
      "out vec4 vertexColorGSOutput;");
    vtkShaderProgram::Substitute(
      GSSource, "//VTK::Color::Impl", "vertexColorGSOutput = vertexColorVSOutput[i];");

    colorDec += "in vec4 vertexColorVSOutput;\n";
    colorImpl += "  vec3 ambientColor = ambientIntensity * vertexColorVSOutput.rgb;\n"
                 "  vec3 diffuseColor = diffuseIntensity * vertexColorVSOutput.rgb;\n"
                 "  float opacity = opacityUniform * vertexColorVSOutput.a;";
  }
  // handle point color texture map coloring
  else if (this->InterpolateScalarsBeforeMapping && this->ColorCoordinates &&
    !this->DrawingEdgesOrVertices)
  {
    colorImpl += "  vec4 texColor = texture(colortexture, tcoordVCVSOutput.st);\n"
                 "  vec3 ambientColor = ambientIntensity * texColor.rgb;\n"
                 "  vec3 diffuseColor = diffuseIntensity * texColor.rgb;\n"
                 "  float opacity = opacityUniform * texColor.a;";
  }
  // are we doing cell scalar coloring by texture?
  else if (this->HaveCellScalars && !this->DrawingEdgesOrVertices && !pointPicking)
  {
    colorImpl +=
      "  vec4 texColor = texelFetchBuffer(textureC, gl_PrimitiveID + PrimitiveIDOffset);\n"
      "  vec3 ambientColor = ambientIntensity * texColor.rgb;\n"
      "  vec3 diffuseColor = diffuseIntensity * texColor.rgb;\n"
      "  float opacity = opacityUniform * texColor.a;";
  }
  // just material but handle backfaceproperties
  else
  {
    colorImpl += "  vec3 ambientColor = ambientIntensity * ambientColorUniform;\n"
                 "  vec3 diffuseColor = diffuseIntensity * diffuseColorUniform;\n"
                 "  float opacity = opacityUniform;\n";

    if (actor->GetBackfaceProperty() && !this->DrawingEdgesOrVertices)
    {
      colorDec += "uniform float opacityUniformBF; // the fragment opacity\n"
                  "uniform float ambientIntensityBF; // the material ambient\n"
                  "uniform float diffuseIntensityBF; // the material diffuse\n"
                  "uniform vec3 ambientColorUniformBF; // ambient material color\n"
                  "uniform vec3 diffuseColorUniformBF; // diffuse material color\n";
      if (this->LastLightComplexity[this->LastBoundBO])
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

  if (this->HaveCellScalars && !this->DrawingEdgesOrVertices)
  {
    colorDec += "uniform samplerBuffer textureC;\n";
  }

  vtkShaderProgram::Substitute(FSSource, "//VTK::Color::Dec", colorDec);
  vtkShaderProgram::Substitute(FSSource, "//VTK::Color::Impl", colorImpl);

  shaders[vtkShader::Vertex]->SetSource(VSSource);
  shaders[vtkShader::Geometry]->SetSource(GSSource);
  shaders[vtkShader::Fragment]->SetSource(FSSource);
}

void vtkOpenGLPolyDataMapper::ReplaceShaderLight(
  std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* actor)
{
  std::string FSSource = shaders[vtkShader::Fragment]->GetSource();
  std::ostringstream toString;

  // check for normal rendering
  vtkInformation* info = actor->GetPropertyKeys();
  if (info && info->Has(vtkLightingMapPass::RENDER_NORMALS()))
  {
    vtkShaderProgram::Substitute(FSSource, "//VTK::Light::Impl",
      "  vec3 n = (normalVCVSOutput + 1.0) * 0.5;\n"
      "  gl_FragData[0] = vec4(n.x, n.y, n.z, 1.0);");
    shaders[vtkShader::Fragment]->SetSource(FSSource);
    return;
  }

  // If rendering, set diffuse and specular colors to pure white
  if (info && info->Has(vtkLightingMapPass::RENDER_LUMINANCE()))
  {
    vtkShaderProgram::Substitute(FSSource, "//VTK::Light::Impl",
      "  diffuseColor = vec3(1, 1, 1);\n"
      "  specularColor = vec3(1, 1, 1);\n"
      "  //VTK::Light::Impl\n",
      false);
  }

  int lastLightComplexity = this->LastLightComplexity[this->LastBoundBO];
  int lastLightCount = this->LastLightCount[this->LastBoundBO];

  if (actor->GetProperty()->GetInterpolation() != VTK_PBR && lastLightCount == 0)
  {
    lastLightComplexity = 0;
  }

  bool hasIBL = false;

  if (actor->GetProperty()->GetInterpolation() == VTK_PBR && lastLightComplexity > 0)
  {
    vtkShaderProgram::Substitute(FSSource, "//VTK::Light::Dec",
      "//VTK::Light::Dec\n"
      "uniform mat3 normalMatrix;\n" // move to normal code
      "const float PI = 3.14159265359;\n"
      "const float recPI = 0.31830988618;\n"
      "uniform float metallicUniform;\n"
      "uniform float roughnessUniform;\n"
      "uniform vec3 emissiveFactorUniform;\n"
      "uniform float aoStrengthUniform;\n\n"
      "float D_GGX(float NdH, float roughness)\n"
      "{\n"
      "  float a = roughness * roughness;\n"
      "  float a2 = a * a;\n"
      "  float d = (NdH * a2 - NdH) * NdH + 1.0;\n"
      "  return a2 / (PI * d * d);\n"
      "}\n"
      "float V_SmithCorrelated(float NdV, float NdL, float roughness)\n"
      "{\n"
      "  float a2 = roughness * roughness;\n"
      "  float ggxV = NdL * sqrt(a2 + NdV * (NdV - a2 * NdV));\n"
      "  float ggxL = NdV * sqrt(a2 + NdL * (NdL - a2 * NdL));\n"
      "  return 0.5 / (ggxV + ggxL);\n"
      "}\n"
      "vec3 F_Schlick(float HdV, vec3 F0)\n"
      "{\n"
      "  return F0 + (1.0 - F0) * pow(1.0 - HdV, 5.0);\n"
      "}\n"
      "vec3 F_SchlickRoughness(float HdV, vec3 F0, float roughness)\n"
      "{\n"
      "  return F0 + (1.0 - F0) * (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - HdV, 5.0);\n"
      "}\n"
      "vec3 DiffuseLambert(vec3 albedo)\n"
      "{\n"
      "  return albedo * recPI;\n"
      "}\n",
      false);

    // disable default behavior with textures
    vtkShaderProgram::Substitute(FSSource, "//VTK::TCoord::Impl", "");

    // get color and material from textures
    std::vector<texinfo> textures = this->GetTextures(actor);
    bool albedo = false;
    bool material = false;
    bool emissive = false;
    toString.clear();

    if (this->HaveTCoords(this->CurrentInput) && !this->DrawingEdgesOrVertices)
    {
      for (auto& t : textures)
      {
        if (t.second == "albedoTex")
        {
          albedo = true;
          toString << "vec4 albedoSample = texture(albedoTex, tcoordVCVSOutput);\n"
                      "  vec3 albedo = albedoSample.rgb * diffuseColor;\n"
                      "  opacity = albedoSample.a;\n";
        }
        else if (t.second == "materialTex")
        {
          // we are using GLTF specification here with a combined texture holding values for AO,
          // roughness and metallic on R,G,B channels respectively
          material = true;
          toString << "  vec4 material = texture(materialTex, tcoordVCVSOutput);\n"
                      "  float roughness = material.g * roughnessUniform;\n"
                      "  float metallic = material.b * metallicUniform;\n"
                      "  float ao = material.r;\n";
        }
        else if (t.second == "emissiveTex")
        {
          emissive = true;
          toString << "  vec3 emissiveColor = texture(emissiveTex, tcoordVCVSOutput).rgb;\n"
                      "  emissiveColor = emissiveColor * emissiveFactorUniform;\n";
        }
      }
    }

    // IBL
    if (ren->GetUseImageBasedLighting() && ren->GetEnvironmentCubeMap())
    {
      vtkOpenGLRenderer* oglRen = vtkOpenGLRenderer::SafeDownCast(ren);
      if (oglRen)
      {
        hasIBL = true;
        toString << "  const float prefilterMaxLevel = float("
                 << (oglRen->GetEnvMapPrefiltered()->GetPrefilterLevels() - 1) << ");\n";
      }
    }

    if (!albedo)
    {
      toString << "vec3 albedo = pow(diffuseColor, vec3(2.2));\n"; // to linear color space
    }
    if (!material)
    {
      toString << "  float roughness = roughnessUniform;\n";
      toString << "  float metallic = metallicUniform;\n";
      toString << "  float ao = 1.0;\n";
    }
    if (!emissive)
    {
      toString << "  vec3 emissiveColor = vec3(0.0);\n";
    }

    toString << "  vec3 N = normalVCVSOutput;\n"
                "  vec3 V = normalize(-vertexVC.xyz);\n"
                "  float NdV = clamp(dot(N, V), 1e-5, 1.0);\n";

    if (hasIBL)
    {
      toString << "  vec3 irradiance = texture(irradianceTex, inverse(normalMatrix)*N).rgb;\n";
      toString << "  vec3 worldReflect = normalize(inverse(normalMatrix)*reflect(-V, N));\n"
                  "  vec3 prefilteredColor = textureLod(prefilterTex, worldReflect,"
                  " roughness * prefilterMaxLevel).rgb;\n";
      toString << "  vec2 brdf = texture(brdfTex, vec2(NdV, roughness)).rg;\n";
    }
    else
    {
      toString << "  vec3 irradiance = vec3(0.03);\n";
      toString << "  vec3 prefilteredColor = vec3(0.03);\n";
      toString << "  vec2 brdf = vec2(0.0, 0.0);\n";
    }

    toString << "  vec3 Lo = vec3(0.0);\n";

    if (lastLightComplexity != 0)
    {
      toString << "  vec3 F0 = mix(vec3(0.04), albedo, metallic);\n"
                  "  vec3 L, H, radiance, F, specular, diffuse;\n"
                  "  float NdL, NdH, HdV, distanceVC, attenuation, D, Vis;\n\n";
    }

    toString << "//VTK::Light::Impl\n";

    vtkShaderProgram::Substitute(FSSource, "//VTK::Light::Impl", toString.str(), false);
    toString.clear();
    toString.str("");

    if (hasIBL)
    {
      // add uniforms
      vtkShaderProgram::Substitute(FSSource, "//VTK::Light::Dec",
        "//VTK::Light::Dec\n"
        "uniform sampler2D brdfTex;\n"
        "uniform samplerCube irradianceTex;\n"
        "uniform samplerCube prefilterTex;\n");
    }
  }

  // get Standard Lighting Decls
  vtkShaderProgram::Substitute(
    FSSource, "//VTK::Light::Dec", static_cast<vtkOpenGLRenderer*>(ren)->GetLightingUniforms());

  switch (lastLightComplexity)
  {
    case 0: // no lighting or RENDER_VALUES
      vtkShaderProgram::Substitute(FSSource, "//VTK::Light::Impl",
        "  gl_FragData[0] = vec4(ambientColor + diffuseColor, opacity);\n"
        "  //VTK::Light::Impl\n",
        false);
      break;

    case 1: // headlight
      if (actor->GetProperty()->GetInterpolation() == VTK_PBR)
      {
        // L = V = H for headlights
        toString << "  NdV = clamp(dot(N, V), 1e-5, 1.0);\n"
                    "  D = D_GGX(NdV, roughness);\n"
                    "  Vis = V_SmithCorrelated(NdV, NdV, roughness);\n"
                    "  F = F_Schlick(1.0, F0);\n"
                    "  specular = D * Vis * F;\n"
                    "  diffuse = (1.0 - metallic) * (1.0 - F) * DiffuseLambert(albedo);\n"
                    "  Lo += (diffuse + specular) * lightColor0 * NdV;\n\n"
                    "//VTK::Light::Impl\n";
      }
      else
      {
        toString << "  float df = max(0.0,normalVCVSOutput.z);\n"
                    "  float sf = pow(df, specularPower);\n"
                    "  vec3 diffuse = df * diffuseColor * lightColor0;\n"
                    "  vec3 specular = sf * specularColor * lightColor0;\n"
                    "  gl_FragData[0] = vec4(ambientColor + diffuse + specular, opacity);\n"
                    "  //VTK::Light::Impl\n";
      }

      vtkShaderProgram::Substitute(FSSource, "//VTK::Light::Impl", toString.str(), false);
      break;

    case 2: // light kit
      toString.clear();
      toString.str("");

      if (actor->GetProperty()->GetInterpolation() == VTK_PBR)
      {
        for (int i = 0; i < lastLightCount; ++i)
        {
          toString << "  L = normalize(-lightDirectionVC" << i
                   << ");\n"
                      "  H = normalize(V + L);\n"
                      "  NdL = clamp(dot(N, L), 1e-5, 1.0);\n"
                      "  NdH = clamp(dot(N, H), 1e-5, 1.0);\n"
                      "  HdV = clamp(dot(H, V), 1e-5, 1.0);\n"
                      "  radiance = lightColor"
                   << i
                   << ";\n"
                      "  D = D_GGX(NdH, roughness);\n"
                      "  Vis = V_SmithCorrelated(NdV, NdL, roughness);\n"
                      "  F = F_Schlick(HdV, F0);\n"
                      "  specular = D * Vis * F;\n"
                      "  diffuse = (1.0 - metallic) * (1.0 - F) * DiffuseLambert(albedo);\n"
                      "  Lo += (diffuse + specular) * radiance * NdL;\n";
        }
        toString << "//VTK::Light::Impl\n";
      }
      else
      {
        toString << "  vec3 diffuse = vec3(0,0,0);\n"
                    "  vec3 specular = vec3(0,0,0);\n"
                    "  float df;\n"
                    "  float sf;\n";
        for (int i = 0; i < lastLightCount; ++i)
        {
          toString << "    df = max(0.0, dot(normalVCVSOutput, -lightDirectionVC" << i
                   << "));\n"
                      // if you change the next line also change vtkShadowMapPass
                      "  diffuse += (df * lightColor"
                   << i << ");\n"
                   << "  sf = sign(df)*pow(max(0.0, dot( reflect(lightDirectionVC" << i
                   << ", normalVCVSOutput), normalize(-vertexVC.xyz))), specularPower);\n"
                      // if you change the next line also change vtkShadowMapPass
                      "  specular += (sf * lightColor"
                   << i << ");\n";
        }
        toString << "  diffuse = diffuse * diffuseColor;\n"
                    "  specular = specular * specularColor;\n"
                    "  gl_FragData[0] = vec4(ambientColor + diffuse + specular, opacity);"
                    "  //VTK::Light::Impl";
      }

      vtkShaderProgram::Substitute(FSSource, "//VTK::Light::Impl", toString.str(), false);
      break;

    case 3: // positional
      toString.clear();
      toString.str("");

      if (actor->GetProperty()->GetInterpolation() == VTK_PBR)
      {
        for (int i = 0; i < lastLightCount; ++i)
        {
          toString << "  L = lightPositionVC" << i
                   << " - vertexVC.xyz;\n"
                      "  distanceVC = length(L);\n"
                      "  L = normalize(L);\n"
                      "  H = normalize(V + L);\n"
                      "  NdL = clamp(dot(N, L), 1e-5, 1.0);\n"
                      "  NdH = clamp(dot(N, H), 1e-5, 1.0);\n"
                      "  HdV = clamp(dot(H, V), 1e-5, 1.0);\n"
                      "  if (lightPositional"
                   << i
                   << " == 0)\n"
                      "  {\n"
                      "    attenuation = 1.0;\n"
                      "  }\n"
                      "  else\n"
                      "  {\n"
                      "    attenuation = 1.0 / (lightAttenuation"
                   << i
                   << ".x\n"
                      "      + lightAttenuation"
                   << i
                   << ".y * distanceVC\n"
                      "      + lightAttenuation"
                   << i
                   << ".z * distanceVC * distanceVC);\n"
                      "    // cone angle is less than 90 for a spot light\n"
                      "    if (lightConeAngle"
                   << i
                   << " < 90.0) {\n"
                      "      float coneDot = dot(-L, lightDirectionVC"
                   << i
                   << ");\n"
                      "      // if inside the cone\n"
                      "      if (coneDot >= cos(radians(lightConeAngle"
                   << i
                   << ")))\n"
                      "      {\n"
                      "        attenuation = attenuation * pow(coneDot, lightExponent"
                   << i
                   << ");\n"
                      "      }\n"
                      "      else\n"
                      "      {\n"
                      "        attenuation = 0.0;\n"
                      "      }\n"
                      "    }\n"
                      "  }\n"
                      "  radiance = lightColor"
                   << i
                   << " * attenuation;\n"
                      "  D = D_GGX(NdH, roughness);\n"
                      "  Vis = V_SmithCorrelated(NdV, NdL, roughness);\n"
                      "  F = F_Schlick(HdV, F0);\n"
                      "  specular = D * Vis * F;\n"
                      "  diffuse = (1.0 - metallic) * (1.0 - F) * DiffuseLambert(albedo);\n"
                      "  Lo += (diffuse + specular) * radiance * NdL;\n\n";
        }
        toString << "//VTK::Light::Impl\n";
      }
      else
      {
        toString << "  vec3 diffuse = vec3(0,0,0);\n"
                    "  vec3 specular = vec3(0,0,0);\n"
                    "  vec3 vertLightDirectionVC;\n"
                    "  float attenuation;\n"
                    "  float df;\n"
                    "  float sf;\n";
        for (int i = 0; i < lastLightCount; ++i)
        {
          toString
            << "    attenuation = 1.0;\n"
               "    if (lightPositional"
            << i
            << " == 0) {\n"
               "      vertLightDirectionVC = lightDirectionVC"
            << i
            << "; }\n"
               "    else {\n"
               "      vertLightDirectionVC = vertexVC.xyz - lightPositionVC"
            << i
            << ";\n"
               "      float distanceVC = length(vertLightDirectionVC);\n"
               "      vertLightDirectionVC = normalize(vertLightDirectionVC);\n"
               "      attenuation = 1.0 /\n"
               "        (lightAttenuation"
            << i
            << ".x\n"
               "         + lightAttenuation"
            << i
            << ".y * distanceVC\n"
               "         + lightAttenuation"
            << i
            << ".z * distanceVC * distanceVC);\n"
               "      // cone angle is less than 90 for a spot light\n"
               "      if (lightConeAngle"
            << i
            << " < 90.0) {\n"
               "        float coneDot = dot(vertLightDirectionVC, lightDirectionVC"
            << i
            << ");\n"
               "        // if inside the cone\n"
               "        if (coneDot >= cos(radians(lightConeAngle"
            << i
            << "))) {\n"
               "          attenuation = attenuation * pow(coneDot, lightExponent"
            << i
            << "); }\n"
               "        else {\n"
               "          attenuation = 0.0; }\n"
               "        }\n"
               "      }\n"
            << "    df = max(0.0,attenuation*dot(normalVCVSOutput, -vertLightDirectionVC));\n"
               // if you change the next line also change vtkShadowMapPass
               "    diffuse += (df * lightColor"
            << i
            << ");\n"
               "    sf = sign(df)*attenuation*pow( max(0.0, dot( reflect(vertLightDirectionVC, "
               "normalVCVSOutput), normalize(-vertexVC.xyz))), specularPower);\n"
               // if you change the next line also change vtkShadowMapPass
               "      specular += (sf * lightColor"
            << i << ");\n";
        }
        toString << "  diffuse = diffuse * diffuseColor;\n"
                    "  specular = specular * specularColor;\n"
                    "  gl_FragData[0] = vec4(ambientColor + diffuse + specular, opacity);"
                    "  //VTK::Light::Impl";
      }
      vtkShaderProgram::Substitute(FSSource, "//VTK::Light::Impl", toString.str(), false);
      break;
  }

  if (actor->GetProperty()->GetInterpolation() == VTK_PBR && lastLightComplexity > 0)
  {
    vtkShaderProgram::Substitute(FSSource, "//VTK::Light::Impl",
      "  vec3 kS = F_SchlickRoughness(max(NdV, 0.0), F0, roughness);\n"
      "  vec3 kD = 1.0 - kS;\n"
      "  kD *= 1.0 - metallic;\n" // no diffuse for metals
      "  vec3 ambient = (kD * irradiance * albedo + prefilteredColor * (kS * brdf.r + brdf.g));\n"
      "  vec3 color = ambient + Lo;\n"
      "  color = mix(color, color * ao, aoStrengthUniform);\n" // ambient occlusion
      "  color += emissiveColor;\n"                            // emissive
      "  color = pow(color, vec3(1.0/2.2));\n"                 // to sRGB color space
      "  gl_FragData[0] = vec4(color, opacity);\n"
      "  //VTK::Light::Impl",
      false);
  }

  // If rendering luminance values, write those values to the fragment
  if (info && info->Has(vtkLightingMapPass::RENDER_LUMINANCE()))
  {
    switch (this->LastLightComplexity[this->LastBoundBO])
    {
      case 0: // no lighting
        vtkShaderProgram::Substitute(
          FSSource, "//VTK::Light::Impl", "  gl_FragData[0] = vec4(0.0, 0.0, 0.0, 1.0);");
        break;
      case 1: // headlight
      case 2: // light kit
      case 3: // positional
        vtkShaderProgram::Substitute(FSSource, "//VTK::Light::Impl",
          "  float ambientY = dot(vec3(0.2126, 0.7152, 0.0722), ambientColor);\n"
          "  gl_FragData[0] = vec4(ambientY, diffuse.x, specular.x, 1.0);");
        break;
    }
  }

  shaders[vtkShader::Fragment]->SetSource(FSSource);
}

void vtkOpenGLPolyDataMapper::ReplaceShaderTCoord(
  std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer*, vtkActor* actor)
{
  if (this->DrawingEdgesOrVertices)
  {
    return;
  }

  std::vector<texinfo> textures = this->GetTextures(actor);
  if (textures.empty())
  {
    return;
  }

  std::string VSSource = shaders[vtkShader::Vertex]->GetSource();
  std::string GSSource = shaders[vtkShader::Geometry]->GetSource();
  std::string FSSource = shaders[vtkShader::Fragment]->GetSource();

  // always define texture maps if we have them
  std::string tMapDecFS;
  for (auto it : textures)
  {
    if (it.first->GetCubeMap())
    {
      tMapDecFS += "uniform samplerCube ";
    }
    else
    {
      tMapDecFS += "uniform sampler2D ";
    }
    tMapDecFS += it.second + ";\n";
  }
  vtkShaderProgram::Substitute(FSSource, "//VTK::TMap::Dec", tMapDecFS);

  // now handle each texture coordinate
  std::set<std::string> tcoordnames;
  for (auto it : textures)
  {
    // do we have special tcoords for this texture?
    std::string tcoordname = this->GetTextureCoordinateName(it.second.c_str());
    int tcoordComps = this->VBOs->GetNumberOfComponents(tcoordname.c_str());
    if (tcoordComps == 1 || tcoordComps == 2)
    {
      tcoordnames.insert(tcoordname);
    }
  }

  // if no texture coordinates then we are done
  if (tcoordnames.empty())
  {
    shaders[vtkShader::Vertex]->SetSource(VSSource);
    shaders[vtkShader::Geometry]->SetSource(GSSource);
    shaders[vtkShader::Fragment]->SetSource(FSSource);
    return;
  }

  // handle texture transformation matrix and create the
  // vertex shader texture coordinate implementation
  // code for all texture coordinates.
  vtkInformation* info = actor->GetPropertyKeys();
  std::string vsimpl;
  if (info && info->Has(vtkProp::GeneralTextureTransform()))
  {
    vtkShaderProgram::Substitute(VSSource, "//VTK::TCoord::Dec",
      "//VTK::TCoord::Dec\n"
      "uniform mat4 tcMatrix;",
      false);
    for (const auto& it : tcoordnames)
    {
      int tcoordComps = this->VBOs->GetNumberOfComponents(it.c_str());
      if (tcoordComps == 1)
      {
        vsimpl = vsimpl + "vec4 " + it + "Tmp = tcMatrix*vec4(" + it + ",0.0,0.0,1.0);\n" + it +
          "VCVSOutput = " + it + "Tmp.x/" + it + "Tmp.w;\n";
        if (this->SeamlessU)
        {
          vsimpl += it + "VCVSOutputU1 = fract(" + it + "VCVSOutput.x);\n" + it +
            "VCVSOutputU2 = fract(" + it + "VCVSOutput.x+0.5)-0.5;\n";
        }
      }
      else
      {
        vsimpl = vsimpl + "vec4 " + it + "Tmp = tcMatrix*vec4(" + it + ",0.0,1.0);\n" + it +
          "VCVSOutput = " + it + "Tmp.xy/" + it + "Tmp.w;\n";
        if (this->SeamlessU)
        {
          vsimpl += it + "VCVSOutputU1 = fract(" + it + "VCVSOutput.x);\n" + it +
            "VCVSOutputU2 = fract(" + it + "VCVSOutput.x+0.5)-0.5;\n";
        }
        if (this->SeamlessV)
        {
          vsimpl += it + "VCVSOutputV1 = fract(" + it + "VCVSOutput.y);\n" + it +
            "VCVSOutputV2 = fract(" + it + "VCVSOutput.y+0.5)-0.5;\n";
        }
      }
    }
  }
  else
  {
    for (const auto& it : tcoordnames)
    {
      vsimpl = vsimpl + it + "VCVSOutput = " + it + ";\n";
      if (this->SeamlessU)
      {
        vsimpl += it + "VCVSOutputU1 = fract(" + it + "VCVSOutput.x);\n" + it +
          "VCVSOutputU2 = fract(" + it + "VCVSOutput.x+0.5)-0.5;\n";
      }
      if (this->SeamlessV)
      {
        vsimpl += it + "VCVSOutputV1 = fract(" + it + "VCVSOutput.y);\n" + it +
          "VCVSOutputV2 = fract(" + it + "VCVSOutput.y+0.5)-0.5;\n";
      }
    }
  }

  vtkShaderProgram::Substitute(VSSource, "//VTK::TCoord::Impl", vsimpl);

  // now create the rest of the vertex and geometry shader code
  std::string vsdec;
  std::string gsdec;
  std::string gsimpl;
  std::string fsdec;
  for (const auto& it : tcoordnames)
  {
    int tcoordComps = this->VBOs->GetNumberOfComponents(it.c_str());
    std::string tCoordType;
    if (tcoordComps == 1)
    {
      tCoordType = "float";
    }
    else
    {
      tCoordType = "vec2";
    }
    vsdec = vsdec + "in " + tCoordType + " " + it + ";\n";
    vsdec = vsdec + "out " + tCoordType + " " + it + "VCVSOutput;\n";
    if (this->SeamlessU)
    {
      vsdec = vsdec + "out float " + it + "VCVSOutputU1;\n";
      vsdec = vsdec + "out float " + it + "VCVSOutputU2;\n";
    }
    if (this->SeamlessV && tcoordComps > 1)
    {
      vsdec = vsdec + "out float " + it + "VCVSOutputV1;\n";
      vsdec = vsdec + "out float " + it + "VCVSOutputV2;\n";
    }
    gsdec = gsdec + "in " + tCoordType + " " + it + "VCVSOutput[];\n";
    gsdec = gsdec + "out " + tCoordType + " " + it + "VCGSOutput;\n";
    gsimpl = gsimpl + it + "VCGSOutput = " + it + "VCVSOutput[i];\n";
    fsdec = fsdec + "in " + tCoordType + " " + it + "VCVSOutput;\n";
    if (this->SeamlessU)
    {
      fsdec = fsdec + "in float " + it + "VCVSOutputU1;\n";
      fsdec = fsdec + "in float " + it + "VCVSOutputU2;\n";
    }
    if (this->SeamlessV && tcoordComps > 1)
    {
      fsdec = fsdec + "in float " + it + "VCVSOutputV1;\n";
      fsdec = fsdec + "in float " + it + "VCVSOutputV2;\n";
    }
  }

  vtkShaderProgram::Substitute(VSSource, "//VTK::TCoord::Dec", vsdec);
  vtkShaderProgram::Substitute(GSSource, "//VTK::TCoord::Dec", gsdec);
  vtkShaderProgram::Substitute(GSSource, "//VTK::TCoord::Impl", gsimpl);
  vtkShaderProgram::Substitute(FSSource, "//VTK::TCoord::Dec", fsdec);

  int nbTex2d = 0;

  // OK now handle the fragment shader implementation
  // everything else has been done.
  std::string tCoordImpFS;
  for (size_t i = 0; i < textures.size(); ++i)
  {
    vtkTexture* texture = textures[i].first;

    // ignore cubemaps
    if (texture->GetCubeMap())
    {
      continue;
    }

    // ignore special textures
    if (textures[i].second == "albedoTex" || textures[i].second == "normalTex" ||
      textures[i].second == "materialTex" || textures[i].second == "brdfTex" ||
      textures[i].second == "emissiveTex")
    {
      continue;
    }

    nbTex2d++;

    std::stringstream ss;

    // do we have special tcoords for this texture?
    std::string tcoordname = this->GetTextureCoordinateName(textures[i].second.c_str());
    int tcoordComps = this->VBOs->GetNumberOfComponents(tcoordname.c_str());

    std::string tCoordImpFSPre;
    std::string tCoordImpFSPost;
    if (tcoordComps == 1)
    {
      tCoordImpFSPre = "vec2(";
      tCoordImpFSPost = ", 0.0)";
    }
    else
    {
      tCoordImpFSPre = "";
      tCoordImpFSPost = "";
    }

    // Read texture color
    if (this->SeamlessU || (this->SeamlessV && tcoordComps > 1))
    {
      // Implementation of "Cylindrical and Toroidal Parameterizations Without Vertex Seams"
      // Marco Turini, 2011
      if (tcoordComps == 1)
      {
        ss << "  float texCoord;\n";
      }
      else
      {
        ss << "  vec2 texCoord;\n";
      }
      if (this->SeamlessU)
      {
        ss << "  if (fwidth(" << tCoordImpFSPre << tcoordname << "VCVSOutputU1" << tCoordImpFSPost
           << ") <= fwidth(" << tCoordImpFSPre << tcoordname << "VCVSOutputU2" << tCoordImpFSPost
           << "))\n  {\n"
           << "    texCoord.x = " << tCoordImpFSPre << tcoordname << "VCVSOutputU1"
           << tCoordImpFSPost << ";\n  }\n  else\n  {\n"
           << "    texCoord.x = " << tCoordImpFSPre << tcoordname << "VCVSOutputU2"
           << tCoordImpFSPost << ";\n  }\n";
      }
      else
      {
        ss << "  texCoord.x = " << tCoordImpFSPre << tcoordname << "VCVSOutput" << tCoordImpFSPost
           << ".x"
           << ";\n";
      }
      if (tcoordComps > 1)
      {
        if (this->SeamlessV)
        {
          ss << "  if (fwidth(" << tCoordImpFSPre << tcoordname << "VCVSOutputV1" << tCoordImpFSPost
             << ") <= fwidth(" << tCoordImpFSPre << tcoordname << "VCVSOutputV2" << tCoordImpFSPost
             << "))\n  {\n"
             << "    texCoord.y = " << tCoordImpFSPre << tcoordname << "VCVSOutputV1"
             << tCoordImpFSPost << ";\n  }\n  else\n  {\n"
             << "    texCoord.y = " << tCoordImpFSPre << tcoordname << "VCVSOutputV2"
             << tCoordImpFSPost << ";\n  }\n";
        }
        else
        {
          ss << "  texCoord.y = " << tCoordImpFSPre << tcoordname << "VCVSOutput" << tCoordImpFSPost
             << ".y"
             << ";\n";
        }
      }
      ss << "  vec4 tcolor_" << i << " = texture(" << textures[i].second
         << ", texCoord); // Read texture color\n";
    }
    else
    {
      ss << "vec4 tcolor_" << i << " = texture(" << textures[i].second << ", " << tCoordImpFSPre
         << tcoordname << "VCVSOutput" << tCoordImpFSPost << "); // Read texture color\n";
    }

    // Update color based on texture number of components
    int tNumComp = vtkOpenGLTexture::SafeDownCast(texture)->GetTextureObject()->GetComponents();
    switch (tNumComp)
    {
      case 1:
        ss << "tcolor_" << i << " = vec4(tcolor_" << i << ".r,tcolor_" << i << ".r,tcolor_" << i
           << ".r,1.0)";
        break;
      case 2:
        ss << "tcolor_" << i << " = vec4(tcolor_" << i << ".r,tcolor_" << i << ".r,tcolor_" << i
           << ".r,tcolor_" << i << ".g)";
        break;
      case 3:
        ss << "tcolor_" << i << " = vec4(tcolor_" << i << ".r,tcolor_" << i << ".g,tcolor_" << i
           << ".b,1.0)";
    }
    ss << "; // Update color based on texture nbr of components \n";

    // Define final color based on texture blending
    if (i == 0)
    {
      ss << "vec4 tcolor = tcolor_" << i << "; // BLENDING: None (first texture) \n\n";
    }
    else
    {
      int tBlending = vtkOpenGLTexture::SafeDownCast(texture)->GetBlendingMode();
      switch (tBlending)
      {
        case vtkTexture::VTK_TEXTURE_BLENDING_MODE_REPLACE:
          ss << "tcolor.rgb = tcolor_" << i << ".rgb * tcolor_" << i << ".a + "
             << "tcolor.rgb * (1 - tcolor_" << i << " .a); // BLENDING: Replace\n"
             << "tcolor.a = tcolor_" << i << ".a + tcolor.a * (1 - tcolor_" << i
             << " .a); // BLENDING: Replace\n\n";
          break;
        case vtkTexture::VTK_TEXTURE_BLENDING_MODE_MODULATE:
          ss << "tcolor *= tcolor_" << i << "; // BLENDING: Modulate\n\n";
          break;
        case vtkTexture::VTK_TEXTURE_BLENDING_MODE_ADD:
          ss << "tcolor.rgb = tcolor_" << i << ".rgb * tcolor_" << i << ".a + "
             << "tcolor.rgb * tcolor.a; // BLENDING: Add\n"
             << "tcolor.a += tcolor_" << i << ".a; // BLENDING: Add\n\n";
          break;
        case vtkTexture::VTK_TEXTURE_BLENDING_MODE_ADD_SIGNED:
          ss << "tcolor.rgb = tcolor_" << i << ".rgb * tcolor_" << i << ".a + "
             << "tcolor.rgb * tcolor.a - 0.5; // BLENDING: Add signed\n"
             << "tcolor.a += tcolor_" << i << ".a - 0.5; // BLENDING: Add signed\n\n";
          break;
        case vtkTexture::VTK_TEXTURE_BLENDING_MODE_INTERPOLATE:
          vtkDebugMacro(<< "Interpolate blending mode not supported for OpenGL2 backend.");
          break;
        case vtkTexture::VTK_TEXTURE_BLENDING_MODE_SUBTRACT:
          ss << "tcolor.rgb -= tcolor_" << i << ".rgb * tcolor_" << i
             << ".a; // BLENDING: Subtract\n\n";
          break;
        default:
          vtkDebugMacro(<< "No blending mode given, ignoring this texture colors.");
          ss << "// NO BLENDING MODE: ignoring this texture colors\n";
      }
    }
    tCoordImpFS += ss.str();
  }

  // do texture mapping except for scalar coloring case which is
  // handled in the scalar coloring code
  if (nbTex2d > 0 && (!this->InterpolateScalarsBeforeMapping || !this->ColorCoordinates))
  {
    vtkShaderProgram::Substitute(
      FSSource, "//VTK::TCoord::Impl", tCoordImpFS + "gl_FragData[0] = gl_FragData[0] * tcolor;");
  }

  shaders[vtkShader::Vertex]->SetSource(VSSource);
  shaders[vtkShader::Geometry]->SetSource(GSSource);
  shaders[vtkShader::Fragment]->SetSource(FSSource);
}

void vtkOpenGLPolyDataMapper::ReplaceShaderPicking(
  std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* /* ren */, vtkActor*)
{
  // process actor composite low mid high
  std::string VSSource = shaders[vtkShader::Vertex]->GetSource();
  std::string GSSource = shaders[vtkShader::Geometry]->GetSource();
  std::string FSSource = shaders[vtkShader::Fragment]->GetSource();

  if (this->LastSelectionState >= vtkHardwareSelector::MIN_KNOWN_PASS)
  {
    switch (this->LastSelectionState)
    {
      // point ID low and high are always just gl_VertexId
      case vtkHardwareSelector::POINT_ID_LOW24:
        vtkShaderProgram::Substitute(
          VSSource, "//VTK::Picking::Dec", "flat out int vertexIDVSOutput;\n");
        vtkShaderProgram::Substitute(
          VSSource, "//VTK::Picking::Impl", "  vertexIDVSOutput = gl_VertexID;\n");
        vtkShaderProgram::Substitute(GSSource, "//VTK::Picking::Dec",
          "flat in int vertexIDVSOutput[];\n"
          "flat out int vertexIDGSOutput;");
        vtkShaderProgram::Substitute(
          GSSource, "//VTK::Picking::Impl", "vertexIDGSOutput = vertexIDVSOutput[i];");
        vtkShaderProgram::Substitute(
          FSSource, "//VTK::Picking::Dec", "flat in int vertexIDVSOutput;\n");
        vtkShaderProgram::Substitute(FSSource, "//VTK::Picking::Impl",
          "  int idx = vertexIDVSOutput + 1;\n"
          "  gl_FragData[0] = vec4(float(idx%256)/255.0, float((idx/256)%256)/255.0, "
          "float((idx/65536)%256)/255.0, 1.0);\n");
        break;

      case vtkHardwareSelector::POINT_ID_HIGH24:
        // this may yerk on openGL ES 2.0 so no really huge meshes in ES 2.0 OK
        vtkShaderProgram::Substitute(
          VSSource, "//VTK::Picking::Dec", "flat out int vertexIDVSOutput;\n");
        vtkShaderProgram::Substitute(
          VSSource, "//VTK::Picking::Impl", "  vertexIDVSOutput = gl_VertexID;\n");
        vtkShaderProgram::Substitute(GSSource, "//VTK::Picking::Dec",
          "flat in int vertexIDVSOutput[];\n"
          "flat out int vertexIDGSOutput;");
        vtkShaderProgram::Substitute(
          GSSource, "//VTK::Picking::Impl", "vertexIDGSOutput = vertexIDVSOutput[i];");
        vtkShaderProgram::Substitute(
          FSSource, "//VTK::Picking::Dec", "flat in int vertexIDVSOutput;\n");
        vtkShaderProgram::Substitute(FSSource, "//VTK::Picking::Impl",
          "  int idx = (vertexIDVSOutput + 1);\n idx = ((idx & 0xff000000) >> 24);\n"
          "  gl_FragData[0] = vec4(float(idx)/255.0, 0.0, 0.0, 1.0);\n");
        break;

      // cell ID is just gl_PrimitiveID
      case vtkHardwareSelector::CELL_ID_LOW24:
        vtkShaderProgram::Substitute(FSSource, "//VTK::Picking::Impl",
          "  int idx = gl_PrimitiveID + 1 + PrimitiveIDOffset;\n"
          "  gl_FragData[0] = vec4(float(idx%256)/255.0, float((idx/256)%256)/255.0, "
          "float((idx/65536)%256)/255.0, 1.0);\n");
        break;

      case vtkHardwareSelector::CELL_ID_HIGH24:
        // this may yerk on openGL ES 2.0 so no really huge meshes in ES 2.0 OK
        // if (selector &&
        //     selector->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_POINTS)
        vtkShaderProgram::Substitute(FSSource, "//VTK::Picking::Impl",
          "  int idx = (gl_PrimitiveID + 1 + PrimitiveIDOffset);\n idx = ((idx & 0xff000000) >> "
          "24);\n"
          "  gl_FragData[0] = vec4(float(idx)/255.0, 0.0, 0.0, 1.0);\n");
        break;

      default: // actor process and composite
        vtkShaderProgram::Substitute(FSSource, "//VTK::Picking::Dec", "uniform vec3 mapperIndex;");
        vtkShaderProgram::Substitute(
          FSSource, "//VTK::Picking::Impl", "  gl_FragData[0] = vec4(mapperIndex,1.0);\n");
    }
  }
  shaders[vtkShader::Vertex]->SetSource(VSSource);
  shaders[vtkShader::Geometry]->SetSource(GSSource);
  shaders[vtkShader::Fragment]->SetSource(FSSource);
}

void vtkOpenGLPolyDataMapper::ReplaceShaderClip(
  std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer*, vtkActor*)
{
  std::string VSSource = shaders[vtkShader::Vertex]->GetSource();
  std::string FSSource = shaders[vtkShader::Fragment]->GetSource();
  std::string GSSource = shaders[vtkShader::Geometry]->GetSource();

  if (this->GetNumberOfClippingPlanes())
  {
    // add all the clipping planes
    int numClipPlanes = this->GetNumberOfClippingPlanes();
    if (numClipPlanes > 6)
    {
      vtkErrorMacro(<< "OpenGL has a limit of 6 clipping planes");
    }

    // geometry shader impl
    if (GSSource.length())
    {
      vtkShaderProgram::Substitute(VSSource, "//VTK::Clip::Dec", "out vec4 clipVertexMC;");
      vtkShaderProgram::Substitute(VSSource, "//VTK::Clip::Impl", "  clipVertexMC =  vertexMC;\n");
      vtkShaderProgram::Substitute(GSSource, "//VTK::Clip::Dec",
        "uniform int numClipPlanes;\n"
        "uniform vec4 clipPlanes[6];\n"
        "in vec4 clipVertexMC[];\n"
        "out float clipDistancesGSOutput[6];");
      vtkShaderProgram::Substitute(GSSource, "//VTK::Clip::Impl",
        "for (int planeNum = 0; planeNum < numClipPlanes; planeNum++)\n"
        "  {\n"
        "    clipDistancesGSOutput[planeNum] = dot(clipPlanes[planeNum], clipVertexMC[i]);\n"
        "  }\n");
    }
    else // vertex shader impl
    {
      vtkShaderProgram::Substitute(VSSource, "//VTK::Clip::Dec",
        "uniform int numClipPlanes;\n"
        "uniform vec4 clipPlanes[6];\n"
        "out float clipDistancesVSOutput[6];");
      vtkShaderProgram::Substitute(VSSource, "//VTK::Clip::Impl",
        "for (int planeNum = 0; planeNum < numClipPlanes; planeNum++)\n"
        "    {\n"
        "    clipDistancesVSOutput[planeNum] = dot(clipPlanes[planeNum], vertexMC);\n"
        "    }\n");
    }

    vtkShaderProgram::Substitute(FSSource, "//VTK::Clip::Dec",
      "uniform int numClipPlanes;\n"
      "in float clipDistancesVSOutput[6];");
    vtkShaderProgram::Substitute(FSSource, "//VTK::Clip::Impl",
      "for (int planeNum = 0; planeNum < numClipPlanes; planeNum++)\n"
      "    {\n"
      "    if (clipDistancesVSOutput[planeNum] < 0.0) discard;\n"
      "    }\n");
  }
  shaders[vtkShader::Vertex]->SetSource(VSSource);
  shaders[vtkShader::Fragment]->SetSource(FSSource);
  shaders[vtkShader::Geometry]->SetSource(GSSource);
}

void vtkOpenGLPolyDataMapper::ReplaceShaderNormal(
  std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer*, vtkActor* actor)
{
  std::string FSSource = shaders[vtkShader::Fragment]->GetSource();

  // Render points as spheres if so requested
  // To get the correct zbuffer values we have to
  // adjust the incoming z value based on the shape
  // of the sphere, See the document
  // PixelsToZBufferConversion in this directory for
  // the derivation of the equations used.
  if (this->DrawingSpheres(*this->LastBoundBO, actor))
  {
    vtkShaderProgram::Substitute(FSSource, "//VTK::Normal::Dec",
      "uniform float ZCalcS;\n"
      "uniform float ZCalcR;\n");
    vtkShaderProgram::Substitute(FSSource, "//VTK::Depth::Impl",
      "float xpos = 2.0*gl_PointCoord.x - 1.0;\n"
      "  float ypos = 1.0 - 2.0*gl_PointCoord.y;\n"
      "  float len2 = xpos*xpos+ ypos*ypos;\n"
      "  if (len2 > 1.0) { discard; }\n"
      "  vec3 normalVCVSOutput = normalize(\n"
      "    vec3(2.0*gl_PointCoord.x - 1.0, 1.0 - 2.0*gl_PointCoord.y, sqrt(1.0 - len2)));\n"
      "  gl_FragDepth = gl_FragCoord.z + normalVCVSOutput.z*ZCalcS*ZCalcR;\n"
      "  if (cameraParallel == 0)\n"
      "  {\n"
      "    float ZCalcQ = (normalVCVSOutput.z*ZCalcR - 1.0);\n"
      "    gl_FragDepth = (ZCalcS - gl_FragCoord.z) / ZCalcQ + ZCalcS;\n"
      "  }\n");

    vtkShaderProgram::Substitute(
      FSSource, "//VTK::Normal::Impl", "//Normal computed in Depth::Impl");

    shaders[vtkShader::Fragment]->SetSource(FSSource);
    return;
  }

  // Render lines as tubes if so requested
  // To get the correct zbuffer values we have to
  // adjust the incoming z value based on the shape
  // of the tube, See the document
  // PixelsToZBufferConversion in this directory for
  // the derivation of the equations used.

  // note these are not real tubes. They are wide
  // lines that are fudged a bit to look like tubes
  // this approach is simpler than the OpenGLStickMapper
  // but results in things that are not really tubes
  // for best results use points as spheres with
  // these tubes and make sure the point Width is
  // twice the tube width
  if (this->DrawingTubes(*this->LastBoundBO, actor))
  {
    std::string GSSource = shaders[vtkShader::Geometry]->GetSource();

    vtkShaderProgram::Substitute(FSSource, "//VTK::Normal::Dec",
      "in vec3 tubeBasis1;\n"
      "in vec3 tubeBasis2;\n"
      "uniform float ZCalcS;\n"
      "uniform float ZCalcR;\n");
    vtkShaderProgram::Substitute(FSSource, "//VTK::Depth::Impl",
      "float len2 = tubeBasis1.x*tubeBasis1.x + tubeBasis1.y*tubeBasis1.y;\n"
      "  float lenZ = clamp(sqrt(1.0 - len2),0.0,1.0);\n"
      "  gl_FragDepth = gl_FragCoord.z + lenZ*ZCalcS*ZCalcR/clamp(tubeBasis2.z,0.5,1.0);\n"
      "  if (cameraParallel == 0)\n"
      "  {\n"
      "    float ZCalcQ = (lenZ*ZCalcR/clamp(tubeBasis2.z,0.5,1.0) - 1.0);\n"
      "    gl_FragDepth = (ZCalcS - gl_FragCoord.z) / ZCalcQ + ZCalcS;\n"
      "  }\n");
    vtkShaderProgram::Substitute(FSSource, "//VTK::Normal::Impl",
      "vec3 normalVCVSOutput = normalize(tubeBasis1 + tubeBasis2*lenZ);\n");

    vtkShaderProgram::Substitute(GSSource, "//VTK::Normal::Dec",
      "out vec3 tubeBasis1;\n"
      "out vec3 tubeBasis2;\n");

    vtkShaderProgram::Substitute(GSSource, "//VTK::Normal::Start",
      "vec3 lineDir = normalize(vertexVCVSOutput[1].xyz - vertexVCVSOutput[0].xyz);\n"
      "tubeBasis2 = normalize(cross(lineDir, vec3(normal, 0.0)));\n"
      "tubeBasis2 = tubeBasis2*sign(tubeBasis2.z);\n");

    vtkShaderProgram::Substitute(
      GSSource, "//VTK::Normal::Impl", "tubeBasis1 = 2.0*vec3(normal*((j+1)%2 - 0.5), 0.0);\n");

    shaders[vtkShader::Geometry]->SetSource(GSSource);
    shaders[vtkShader::Fragment]->SetSource(FSSource);
    return;
  }

  if (this->LastLightComplexity[this->LastBoundBO] > 0)
  {
    std::string VSSource = shaders[vtkShader::Vertex]->GetSource();
    std::string GSSource = shaders[vtkShader::Geometry]->GetSource();

    // if we have point normals provided
    if (this->VBOs->GetNumberOfComponents("normalMC") == 3)
    {
      // normal mapping
      std::vector<texinfo> textures = this->GetTextures(actor);
      bool normalTex = std::find_if(textures.begin(), textures.end(), [](const texinfo& tex) {
        return tex.second == "normalTex";
      }) != textures.end();
      if (normalTex && this->VBOs->GetNumberOfComponents("tangentMC") == 3 &&
        !this->DrawingEdgesOrVertices)
      {
        vtkShaderProgram::Substitute(VSSource, "//VTK::Normal::Dec",
          "//VTK::Normal::Dec\n"
          "in vec3 tangentMC;\n"
          "out vec3 tangentVCVSOutput;\n");

        vtkShaderProgram::Substitute(VSSource, "//VTK::Normal::Impl",
          "//VTK::Normal::Impl\n"
          "  tangentVCVSOutput = normalMatrix * tangentMC;\n");

        vtkShaderProgram::Substitute(FSSource, "//VTK::Normal::Dec",
          "//VTK::Normal::Dec\n"
          "uniform float normalScaleUniform;\n"
          "in vec3 tangentVCVSOutput;");

        vtkShaderProgram::Substitute(FSSource, "//VTK::Normal::Impl",
          "//VTK::Normal::Impl\n"
          "  vec3 normalTS = texture(normalTex, tcoordVCVSOutput).xyz * 2.0 - 1.0;\n"
          "  normalTS = normalize(normalTS * vec3(normalScaleUniform, normalScaleUniform, 1.0));\n"
          "  vec3 tangentVC = normalize(tangentVCVSOutput - dot(tangentVCVSOutput, "
          "normalVCVSOutput) * normalVCVSOutput);\n"
          "  vec3 bitangentVC = cross(normalVCVSOutput, tangentVC);\n"
          "  mat3 tbn = mat3(tangentVC, bitangentVC, normalVCVSOutput);\n"
          "  normalVCVSOutput = normalize(tbn * normalTS);\n");
      }
      vtkShaderProgram::Substitute(VSSource, "//VTK::Normal::Dec",
        "in vec3 normalMC;\n"
        "uniform mat3 normalMatrix;\n"
        "out vec3 normalVCVSOutput;");
      vtkShaderProgram::Substitute(
        VSSource, "//VTK::Normal::Impl", "normalVCVSOutput = normalMatrix * normalMC;");
      vtkShaderProgram::Substitute(GSSource, "//VTK::Normal::Dec",
        "in vec3 normalVCVSOutput[];\n"
        "out vec3 normalVCGSOutput;");
      vtkShaderProgram::Substitute(
        GSSource, "//VTK::Normal::Impl", "normalVCGSOutput = normalVCVSOutput[i];");
      vtkShaderProgram::Substitute(FSSource, "//VTK::Normal::Dec", "in vec3 normalVCVSOutput;");
      vtkShaderProgram::Substitute(FSSource, "//VTK::Normal::Impl",
        "vec3 normalVCVSOutput = normalize(normalVCVSOutput);\n"
        //  if (!gl_FrontFacing) does not work in intel hd4000 mac
        //  if (int(gl_FrontFacing) == 0) does not work on mesa
        "  if (gl_FrontFacing == false) { normalVCVSOutput = -normalVCVSOutput; }\n"
        //"normalVC = normalVCVarying;"
      );

      shaders[vtkShader::Vertex]->SetSource(VSSource);
      shaders[vtkShader::Geometry]->SetSource(GSSource);
      shaders[vtkShader::Fragment]->SetSource(FSSource);
      return;
    }

    // OK no point normals, how about cell normals
    if (this->HaveCellNormals)
    {
      vtkShaderProgram::Substitute(FSSource, "//VTK::Normal::Dec",
        "uniform mat3 normalMatrix;\n"
        "uniform samplerBuffer textureN;\n");
      if (this->CellNormalTexture->GetVTKDataType() == VTK_FLOAT)
      {
        vtkShaderProgram::Substitute(FSSource, "//VTK::Normal::Impl",
          "vec3 normalVCVSOutput = \n"
          "    texelFetchBuffer(textureN, gl_PrimitiveID + PrimitiveIDOffset).xyz;\n"
          "normalVCVSOutput = normalize(normalMatrix * normalVCVSOutput);\n"
          "  if (gl_FrontFacing == false) { normalVCVSOutput = -normalVCVSOutput; }\n");
      }
      else
      {
        vtkShaderProgram::Substitute(FSSource, "//VTK::Normal::Impl",
          "vec3 normalVCVSOutput = \n"
          "    texelFetchBuffer(textureN, gl_PrimitiveID + PrimitiveIDOffset).xyz;\n"
          "normalVCVSOutput = normalVCVSOutput * 255.0/127.0 - 1.0;\n"
          "normalVCVSOutput = normalize(normalMatrix * normalVCVSOutput);\n"
          "  if (gl_FrontFacing == false) { normalVCVSOutput = -normalVCVSOutput; }\n");
        shaders[vtkShader::Fragment]->SetSource(FSSource);
        return;
      }
    }

    // OK we have no point or cell normals, so compute something
    // we have a formula for wireframe
    if (actor->GetProperty()->GetRepresentation() == VTK_WIREFRAME)
    {
      // generate a normal for lines, it will be perpendicular to the line
      // and maximally aligned with the camera view direction
      // no clue if this is the best way to do this.
      // the code below has been optimized a bit so what follows is
      // an explanation of the basic approach. Compute the gradient of the line
      // with respect to x and y, the larger of the two
      // cross that with the camera view direction. That gives a vector
      // orthogonal to the camera view and the line. Note that the line and the camera
      // view are probably not orthogonal. Which is why when we cross result that with
      // the line gradient again we get a reasonable normal. It will be othogonal to
      // the line (which is a plane but maximally aligned with the camera view.
      vtkShaderProgram::Substitute(FSSource, "//VTK::UniformFlow::Impl",
        "  vec3 fdx = vec3(dFdx(vertexVC.x),dFdx(vertexVC.y),dFdx(vertexVC.z));\n"
        "  vec3 fdy = vec3(dFdy(vertexVC.x),dFdy(vertexVC.y),dFdy(vertexVC.z));\n"
        // the next two lines deal with some rendering systems
        // that have difficulty computing dfdx/dfdy when they
        // are near zero. Normalization later on can amplify
        // the issue causing rendering artifacts.
        "  if (abs(fdx.x) < 0.000001) { fdx = vec3(0.0);}\n"
        "  if (abs(fdy.y) < 0.000001) { fdy = vec3(0.0);}\n"
        "  //VTK::UniformFlow::Impl\n" // For further replacements
      );
      vtkShaderProgram::Substitute(FSSource, "//VTK::Normal::Impl",
        "vec3 normalVCVSOutput;\n"
        "  fdx = normalize(fdx);\n"
        "  fdy = normalize(fdy);\n"
        "  if (abs(fdx.x) > 0.0)\n"
        "    { normalVCVSOutput = normalize(cross(vec3(fdx.y, -fdx.x, 0.0), fdx)); }\n"
        "  else { normalVCVSOutput = normalize(cross(vec3(fdy.y, -fdy.x, 0.0), fdy));}");
    }
    else // not lines, so surface
    {
      vtkShaderProgram::Substitute(FSSource, "//VTK::UniformFlow::Impl",
        "vec3 fdx = dFdx(vertexVC.xyz);\n"
        "  vec3 fdy = dFdy(vertexVC.xyz);\n"
        "  //VTK::UniformFlow::Impl\n" // For further replacements
      );
      vtkShaderProgram::Substitute(FSSource, "//VTK::Normal::Impl",
        "  vec3 normalVCVSOutput = normalize(cross(fdx,fdy));\n"
        "  if (cameraParallel == 1 && normalVCVSOutput.z < 0.0) { normalVCVSOutput = "
        "-1.0*normalVCVSOutput; }\n"
        "  if (cameraParallel == 0 && dot(normalVCVSOutput,vertexVC.xyz) > 0.0) { normalVCVSOutput "
        "= -1.0*normalVCVSOutput; }");
    }
    shaders[vtkShader::Fragment]->SetSource(FSSource);
  }
}

void vtkOpenGLPolyDataMapper::ReplaceShaderPositionVC(
  std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer*, vtkActor*)
{
  std::string VSSource = shaders[vtkShader::Vertex]->GetSource();
  std::string GSSource = shaders[vtkShader::Geometry]->GetSource();
  std::string FSSource = shaders[vtkShader::Fragment]->GetSource();

  vtkShaderProgram::Substitute(
    FSSource, "//VTK::Camera::Dec", "uniform int cameraParallel;\n", false);

  // do we need the vertex in the shader in View Coordinates
  if (this->LastLightComplexity[this->LastBoundBO] > 0)
  {
    vtkShaderProgram::Substitute(VSSource, "//VTK::PositionVC::Dec", "out vec4 vertexVCVSOutput;");
    vtkShaderProgram::Substitute(VSSource, "//VTK::PositionVC::Impl",
      "vertexVCVSOutput = MCVCMatrix * vertexMC;\n"
      "  gl_Position = MCDCMatrix * vertexMC;\n");
    vtkShaderProgram::Substitute(VSSource, "//VTK::Camera::Dec",
      "uniform mat4 MCDCMatrix;\n"
      "uniform mat4 MCVCMatrix;");
    vtkShaderProgram::Substitute(GSSource, "//VTK::PositionVC::Dec",
      "in vec4 vertexVCVSOutput[];\n"
      "out vec4 vertexVCGSOutput;");
    vtkShaderProgram::Substitute(
      GSSource, "//VTK::PositionVC::Impl", "vertexVCGSOutput = vertexVCVSOutput[i];");
    vtkShaderProgram::Substitute(FSSource, "//VTK::PositionVC::Dec", "in vec4 vertexVCVSOutput;");
    vtkShaderProgram::Substitute(
      FSSource, "//VTK::PositionVC::Impl", "vec4 vertexVC = vertexVCVSOutput;");
  }
  else
  {
    vtkShaderProgram::Substitute(VSSource, "//VTK::Camera::Dec", "uniform mat4 MCDCMatrix;");
    vtkShaderProgram::Substitute(
      VSSource, "//VTK::PositionVC::Impl", "  gl_Position = MCDCMatrix * vertexMC;\n");
  }
  shaders[vtkShader::Vertex]->SetSource(VSSource);
  shaders[vtkShader::Geometry]->SetSource(GSSource);
  shaders[vtkShader::Fragment]->SetSource(FSSource);
}

void vtkOpenGLPolyDataMapper::ReplaceShaderPrimID(
  std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer*, vtkActor*)
{
  std::string GSSource = shaders[vtkShader::Geometry]->GetSource();

  vtkShaderProgram::Substitute(
    GSSource, "//VTK::PrimID::Impl", "gl_PrimitiveID = gl_PrimitiveIDIn;");

  shaders[vtkShader::Geometry]->SetSource(GSSource);
}

void vtkOpenGLPolyDataMapper::ReplaceShaderCoincidentOffset(
  std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* actor)
{
  float factor = 0.0;
  float offset = 0.0;
  this->GetCoincidentParameters(ren, actor, factor, offset);
  vtkOpenGLCamera* cam = (vtkOpenGLCamera*)(ren->GetActiveCamera());

  // if we need an offset handle it here
  // The value of .000016 is suitable for depth buffers
  // of at least 16 bit depth. We do not query the depth
  // right now because we would need some mechanism to
  // cache the result taking into account FBO changes etc.
  if (factor != 0.0 || offset != 0.0)
  {
    std::string FSSource = shaders[vtkShader::Fragment]->GetSource();

    if (cam->GetParallelProjection())
    {
      vtkShaderProgram::Substitute(FSSource, "//VTK::Coincident::Dec", "uniform float cCValue;");
      vtkShaderProgram::Substitute(
        FSSource, "//VTK::Depth::Impl", "gl_FragDepth = gl_FragCoord.z + cCValue;\n");
    }
    else
    {
      vtkShaderProgram::Substitute(FSSource, "//VTK::Coincident::Dec",
        "uniform float cCValue;\n"
        "uniform float cSValue;\n"
        "uniform float cDValue;");
      vtkShaderProgram::Substitute(FSSource, "//VTK::Depth::Impl",
        "float Zdc = gl_FragCoord.z*2.0 - 1.0;\n"
        "  float Z2 = -1.0*cDValue/(Zdc + cCValue) + cSValue;\n"
        "  float Zdc2 = -1.0*cCValue - cDValue/Z2;\n"
        "  gl_FragDepth = Zdc2*0.5 + 0.5;\n");
    }
    shaders[vtkShader::Fragment]->SetSource(FSSource);
  }
}

// If MSAA is enabled, don't write to gl_FragDepth unless we absolutely have
// to. See VTK issue 16899.
void vtkOpenGLPolyDataMapper::ReplaceShaderDepth(
  std::map<vtkShader::Type, vtkShader*>, vtkRenderer*, vtkActor*)
{
  // noop by default
}

void vtkOpenGLPolyDataMapper::ReplaceShaderValues(
  std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* actor)
{
  this->ReplaceShaderRenderPass(shaders, ren, actor, true);
  this->ReplaceShaderCustomUniforms(shaders, actor);
  this->ReplaceShaderColor(shaders, ren, actor);
  this->ReplaceShaderNormal(shaders, ren, actor);
  this->ReplaceShaderLight(shaders, ren, actor);
  this->ReplaceShaderTCoord(shaders, ren, actor);
  this->ReplaceShaderPicking(shaders, ren, actor);
  this->ReplaceShaderClip(shaders, ren, actor);
  this->ReplaceShaderPrimID(shaders, ren, actor);
  this->ReplaceShaderPositionVC(shaders, ren, actor);
  this->ReplaceShaderCoincidentOffset(shaders, ren, actor);
  this->ReplaceShaderDepth(shaders, ren, actor);
  this->ReplaceShaderRenderPass(shaders, ren, actor, false);

  // cout << "VS: " << shaders[vtkShader::Vertex]->GetSource() << endl;
  // cout << "GS: " << shaders[vtkShader::Geometry]->GetSource() << endl;
  // cout << "FS: " << shaders[vtkShader::Fragment]->GetSource() << endl;
}

bool vtkOpenGLPolyDataMapper::DrawingTubesOrSpheres(vtkOpenGLHelper& cellBO, vtkActor* actor)
{
  unsigned int mode =
    this->GetOpenGLMode(actor->GetProperty()->GetRepresentation(), cellBO.PrimitiveType);
  vtkProperty* prop = actor->GetProperty();

  return (prop->GetRenderPointsAsSpheres() && mode == GL_POINTS) ||
    (prop->GetRenderLinesAsTubes() && mode == GL_LINES && prop->GetLineWidth() > 1.0);
}

bool vtkOpenGLPolyDataMapper::DrawingSpheres(vtkOpenGLHelper& cellBO, vtkActor* actor)
{
  return (actor->GetProperty()->GetRenderPointsAsSpheres() &&
    this->GetOpenGLMode(actor->GetProperty()->GetRepresentation(), cellBO.PrimitiveType) ==
      GL_POINTS);
}

bool vtkOpenGLPolyDataMapper::DrawingTubes(vtkOpenGLHelper& cellBO, vtkActor* actor)
{
  return (actor->GetProperty()->GetRenderLinesAsTubes() &&
    actor->GetProperty()->GetLineWidth() > 1.0 &&
    this->GetOpenGLMode(actor->GetProperty()->GetRepresentation(), cellBO.PrimitiveType) ==
      GL_LINES);
}

//-----------------------------------------------------------------------------
bool vtkOpenGLPolyDataMapper::GetNeedToRebuildShaders(
  vtkOpenGLHelper& cellBO, vtkRenderer* ren, vtkActor* actor)
{
  int lightComplexity = 0;
  int numberOfLights = 0;

  // wacky backwards compatibility with old VTK lighting
  // soooo there are many factors that determine if a primitive is lit or not.
  // three that mix in a complex way are representation POINT, Interpolation FLAT
  // and having normals or not.
  bool needLighting = false;
  bool haveNormals = (this->CurrentInput->GetPointData()->GetNormals() != nullptr);
  if (actor->GetProperty()->GetRepresentation() == VTK_POINTS)
  {
    needLighting = (actor->GetProperty()->GetInterpolation() != VTK_FLAT && haveNormals);
  }
  else // wireframe or surface rep
  {
    bool isTrisOrStrips =
      (cellBO.PrimitiveType == PrimitiveTris || cellBO.PrimitiveType == PrimitiveTriStrips);
    needLighting = (isTrisOrStrips ||
      (!isTrisOrStrips && actor->GetProperty()->GetInterpolation() != VTK_FLAT && haveNormals));
  }

  // we sphering or tubing? Yes I made sphere into a verb
  if (this->DrawingTubesOrSpheres(cellBO, actor))
  {
    needLighting = true;
  }

  // do we need lighting?
  if (actor->GetProperty()->GetLighting() && needLighting)
  {
    vtkOpenGLRenderer* oren = static_cast<vtkOpenGLRenderer*>(ren);
    lightComplexity = oren->GetLightingComplexity();
    numberOfLights = oren->GetLightingCount();
  }

  if (this->LastLightComplexity[&cellBO] != lightComplexity ||
    this->LastLightCount[&cellBO] != numberOfLights)
  {
    this->LightComplexityChanged[&cellBO].Modified();
    this->LastLightComplexity[&cellBO] = lightComplexity;
    this->LastLightCount[&cellBO] = numberOfLights;
  }

  // has something changed that would require us to recreate the shader?
  // candidates are
  // -- property modified (representation interpolation and lighting)
  // -- input modified if it changes the presence of normals/tcoords
  // -- light complexity changed
  // -- any render pass that requires it
  // -- some selection state changes
  // we do some quick simple tests first

  // Have the renderpasses changed?
  vtkMTimeType renderPassMTime = this->GetRenderPassStageMTime(actor);

  vtkOpenGLCamera* cam = (vtkOpenGLCamera*)(ren->GetActiveCamera());

  // shape of input data changed?
  float factor, offset;
  this->GetCoincidentParameters(ren, actor, factor, offset);
  unsigned int scv = (this->CurrentInput->GetPointData()->GetNormals() ? 0x01 : 0) +
    (this->HaveCellScalars ? 0x02 : 0) + (this->HaveCellNormals ? 0x04 : 0) +
    ((cam->GetParallelProjection() != 0.0) ? 0x08 : 0) + ((offset != 0.0) ? 0x10 : 0) +
    (this->VBOs->GetNumberOfComponents("scalarColor") ? 0x20 : 0) +
    ((this->VBOs->GetNumberOfComponents("tcoord") % 4) << 6);

  if (cellBO.Program == nullptr || cellBO.ShaderSourceTime < this->GetMTime() ||
    cellBO.ShaderSourceTime < actor->GetProperty()->GetMTime() ||
    cellBO.ShaderSourceTime < actor->GetShaderProperty()->GetShaderMTime() ||
    cellBO.ShaderSourceTime < this->LightComplexityChanged[&cellBO] ||
    cellBO.ShaderSourceTime < this->SelectionStateChanged ||
    cellBO.ShaderSourceTime < renderPassMTime || cellBO.ShaderChangeValue != scv)
  {
    cellBO.ShaderChangeValue = scv;
    return true;
  }

  // if texturing then texture components/blend funcs may have changed
  if (this->VBOs->GetNumberOfComponents("tcoord"))
  {
    vtkMTimeType texMTime = 0;
    std::vector<texinfo> textures = this->GetTextures(actor);
    for (size_t i = 0; i < textures.size(); ++i)
    {
      vtkTexture* texture = textures[i].first;
      texMTime = (texture->GetMTime() > texMTime ? texture->GetMTime() : texMTime);
      if (cellBO.ShaderSourceTime < texMTime)
      {
        return true;
      }
    }
  }

  return false;
}

//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::UpdateShaders(
  vtkOpenGLHelper& cellBO, vtkRenderer* ren, vtkActor* actor)
{
  vtkOpenGLRenderWindow* renWin = vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());

  cellBO.VAO->Bind();
  this->LastBoundBO = &cellBO;

  // has something changed that would require us to recreate the shader?
  if (this->GetNeedToRebuildShaders(cellBO, ren, actor))
  {
    // build the shader source code
    std::map<vtkShader::Type, vtkShader*> shaders;
    vtkShader* vss = vtkShader::New();
    vss->SetType(vtkShader::Vertex);
    shaders[vtkShader::Vertex] = vss;
    vtkShader* gss = vtkShader::New();
    gss->SetType(vtkShader::Geometry);
    shaders[vtkShader::Geometry] = gss;
    vtkShader* fss = vtkShader::New();
    fss->SetType(vtkShader::Fragment);
    shaders[vtkShader::Fragment] = fss;

    this->BuildShaders(shaders, ren, actor);

    // compile and bind the program if needed
    vtkShaderProgram* newShader = renWin->GetShaderCache()->ReadyShaderProgram(shaders);

    vss->Delete();
    fss->Delete();
    gss->Delete();

    // if the shader changed reinitialize the VAO
    if (newShader != cellBO.Program || cellBO.Program->GetMTime() > cellBO.AttributeUpdateTime)
    {
      cellBO.Program = newShader;
      // reset the VAO as the shader has changed
      cellBO.VAO->ReleaseGraphicsResources();
    }

    cellBO.ShaderSourceTime.Modified();
  }
  else
  {
    renWin->GetShaderCache()->ReadyShaderProgram(cellBO.Program);
    if (cellBO.Program->GetMTime() > cellBO.AttributeUpdateTime)
    {
      // reset the VAO as the shader has changed
      cellBO.VAO->ReleaseGraphicsResources();
    }
  }

  if (cellBO.Program)
  {
    this->SetCustomUniforms(cellBO, actor);
    this->SetMapperShaderParameters(cellBO, ren, actor);
    this->SetPropertyShaderParameters(cellBO, ren, actor);
    this->SetCameraShaderParameters(cellBO, ren, actor);
    this->SetLightingShaderParameters(cellBO, ren, actor);

    // allow the program to set what it wants
    this->InvokeEvent(vtkCommand::UpdateShaderEvent, cellBO.Program);
  }

  vtkOpenGLCheckErrorMacro("failed after UpdateShader");
}

void vtkOpenGLPolyDataMapper::SetCustomUniforms(vtkOpenGLHelper& cellBO, vtkActor* actor)
{
  vtkShaderProperty* sp = actor->GetShaderProperty();
  auto vu = static_cast<vtkOpenGLUniforms*>(sp->GetVertexCustomUniforms());
  vu->SetUniforms(cellBO.Program);
  auto fu = static_cast<vtkOpenGLUniforms*>(sp->GetFragmentCustomUniforms());
  fu->SetUniforms(cellBO.Program);
  auto gu = static_cast<vtkOpenGLUniforms*>(sp->GetGeometryCustomUniforms());
  gu->SetUniforms(cellBO.Program);
}

void vtkOpenGLPolyDataMapper::SetMapperShaderParameters(
  vtkOpenGLHelper& cellBO, vtkRenderer* ren, vtkActor* actor)
{

  // Now to update the VAO too, if necessary.
  cellBO.Program->SetUniformi("PrimitiveIDOffset", this->PrimitiveIDOffset);

  if (cellBO.IBO->IndexCount &&
    (this->VBOs->GetMTime() > cellBO.AttributeUpdateTime ||
      cellBO.ShaderSourceTime > cellBO.AttributeUpdateTime ||
      cellBO.VAO->GetMTime() > cellBO.AttributeUpdateTime))
  {
    cellBO.VAO->Bind();

    this->VBOs->AddAllAttributesToVAO(cellBO.Program, cellBO.VAO);

    cellBO.AttributeUpdateTime.Modified();
  }

  // Add IBL textures
  if (ren->GetUseImageBasedLighting() && ren->GetEnvironmentCubeMap())
  {
    vtkOpenGLRenderer* oglRen = vtkOpenGLRenderer::SafeDownCast(ren);
    if (oglRen)
    {
      cellBO.Program->SetUniformi("brdfTex", oglRen->GetEnvMapLookupTable()->GetTextureUnit());
      cellBO.Program->SetUniformi("irradianceTex", oglRen->GetEnvMapIrradiance()->GetTextureUnit());
      cellBO.Program->SetUniformi("prefilterTex", oglRen->GetEnvMapPrefiltered()->GetTextureUnit());
    }
  }

  if (this->HaveTextures(actor))
  {
    std::vector<texinfo> textures = this->GetTextures(actor);
    for (size_t i = 0; i < textures.size(); ++i)
    {
      vtkTexture* texture = textures[i].first;
      if (texture && cellBO.Program->IsUniformUsed(textures[i].second.c_str()))
      {
        int tunit = vtkOpenGLTexture::SafeDownCast(texture)->GetTextureUnit();
        cellBO.Program->SetUniformi(textures[i].second.c_str(), tunit);
      }
    }

    // check for tcoord transform matrix
    vtkInformation* info = actor->GetPropertyKeys();
    vtkOpenGLCheckErrorMacro("failed after Render");
    if (info && info->Has(vtkProp::GeneralTextureTransform()) &&
      cellBO.Program->IsUniformUsed("tcMatrix"))
    {
      double* dmatrix = info->Get(vtkProp::GeneralTextureTransform());
      float fmatrix[16];
      for (int i = 0; i < 4; i++)
      {
        for (int j = 0; j < 4; j++)
        {
          fmatrix[j * 4 + i] = dmatrix[i * 4 + j];
        }
      }
      cellBO.Program->SetUniformMatrix4x4("tcMatrix", fmatrix);
      vtkOpenGLCheckErrorMacro("failed after Render");
    }
  }

  if ((this->HaveCellScalars) && cellBO.Program->IsUniformUsed("textureC"))
  {
    int tunit = this->CellScalarTexture->GetTextureUnit();
    cellBO.Program->SetUniformi("textureC", tunit);
  }

  if (this->HaveCellNormals && cellBO.Program->IsUniformUsed("textureN"))
  {
    int tunit = this->CellNormalTexture->GetTextureUnit();
    cellBO.Program->SetUniformi("textureN", tunit);
  }

  // Handle render pass setup:
  vtkInformation* info = actor->GetPropertyKeys();
  if (info && info->Has(vtkOpenGLRenderPass::RenderPasses()))
  {
    int numRenderPasses = info->Length(vtkOpenGLRenderPass::RenderPasses());
    for (int i = 0; i < numRenderPasses; ++i)
    {
      vtkObjectBase* rpBase = info->Get(vtkOpenGLRenderPass::RenderPasses(), i);
      vtkOpenGLRenderPass* rp = static_cast<vtkOpenGLRenderPass*>(rpBase);
      if (!rp->SetShaderParameters(cellBO.Program, this, actor, cellBO.VAO))
      {
        vtkErrorMacro(
          "RenderPass::SetShaderParameters failed for renderpass: " << rp->GetClassName());
      }
    }
  }

  vtkHardwareSelector* selector = ren->GetSelector();
  if (selector && cellBO.Program->IsUniformUsed("mapperIndex"))
  {
    cellBO.Program->SetUniform3f("mapperIndex", selector->GetPropColorValue());
  }

  if (this->GetNumberOfClippingPlanes() && cellBO.Program->IsUniformUsed("numClipPlanes") &&
    cellBO.Program->IsUniformUsed("clipPlanes"))
  {
    // add all the clipping planes
    int numClipPlanes = this->GetNumberOfClippingPlanes();
    if (numClipPlanes > 6)
    {
      vtkErrorMacro(<< "OpenGL has a limit of 6 clipping planes");
      numClipPlanes = 6;
    }

    double shift[3] = { 0.0, 0.0, 0.0 };
    double scale[3] = { 1.0, 1.0, 1.0 };
    vtkOpenGLVertexBufferObject* vvbo = this->VBOs->GetVBO("vertexMC");
    if (vvbo && vvbo->GetCoordShiftAndScaleEnabled())
    {
      const std::vector<double>& vh = vvbo->GetShift();
      const std::vector<double>& vc = vvbo->GetScale();
      for (int i = 0; i < 3; ++i)
      {
        shift[i] = vh[i];
        scale[i] = vc[i];
      }
    }

    float planeEquations[6][4];
    for (int i = 0; i < numClipPlanes; i++)
    {
      double planeEquation[4];
      this->GetClippingPlaneInDataCoords(actor->GetMatrix(), i, planeEquation);

      // multiply by shift scale if set
      planeEquations[i][0] = planeEquation[0] / scale[0];
      planeEquations[i][1] = planeEquation[1] / scale[1];
      planeEquations[i][2] = planeEquation[2] / scale[2];
      planeEquations[i][3] = planeEquation[3] + planeEquation[0] * shift[0] +
        planeEquation[1] * shift[1] + planeEquation[2] * shift[2];
    }
    cellBO.Program->SetUniformi("numClipPlanes", numClipPlanes);
    cellBO.Program->SetUniform4fv("clipPlanes", 6, planeEquations);
  }

  // handle wide lines
  if (this->HaveWideLines(ren, actor) && cellBO.Program->IsUniformUsed("lineWidthNVC"))
  {
    int vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    float lineWidth[2];
    lineWidth[0] = 2.0 * actor->GetProperty()->GetLineWidth() / vp[2];
    lineWidth[1] = 2.0 * actor->GetProperty()->GetLineWidth() / vp[3];
    cellBO.Program->SetUniform2f("lineWidthNVC", lineWidth);
  }
}

//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::SetLightingShaderParameters(
  vtkOpenGLHelper& cellBO, vtkRenderer* ren, vtkActor*)
{
  // for unlit there are no lighting parameters
  if (this->LastLightComplexity[&cellBO] < 1)
  {
    return;
  }

  vtkShaderProgram* program = cellBO.Program;
  vtkOpenGLRenderer* oren = static_cast<vtkOpenGLRenderer*>(ren);
  oren->UpdateLightingUniforms(program);
}

//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::SetCameraShaderParameters(
  vtkOpenGLHelper& cellBO, vtkRenderer* ren, vtkActor* actor)
{
  vtkShaderProgram* program = cellBO.Program;

  vtkOpenGLCamera* cam = (vtkOpenGLCamera*)(ren->GetActiveCamera());

  // [WMVD]C == {world, model, view, display} coordinates
  // E.g., WCDC == world to display coordinate transformation
  vtkMatrix4x4* wcdc;
  vtkMatrix4x4* wcvc;
  vtkMatrix3x3* norms;
  vtkMatrix4x4* vcdc;
  cam->GetKeyMatrices(ren, wcvc, norms, vcdc, wcdc);

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
    if (this->DrawingSpheres(cellBO, actor))
    {
      program->SetUniformf("ZCalcR",
        actor->GetProperty()->GetPointSize() / (ren->GetSize()[0] * vcdc->GetElement(0, 0)));
    }
    else
    {
      program->SetUniformf("ZCalcR",
        actor->GetProperty()->GetLineWidth() / (ren->GetSize()[0] * vcdc->GetElement(0, 0)));
    }
  }

  // handle coincident
  if (cellBO.Program->IsUniformUsed("cCValue"))
  {
    float diag = actor->GetLength();
    float factor, offset;
    this->GetCoincidentParameters(ren, actor, factor, offset);
    if (cam->GetParallelProjection())
    {
      // one unit of offset is based on 1/1000 of bounding length
      cellBO.Program->SetUniformf("cCValue", -2.0 * 0.001 * diag * offset * vcdc->GetElement(2, 2));
    }
    else
    {
      cellBO.Program->SetUniformf("cCValue", vcdc->GetElement(2, 2));
      cellBO.Program->SetUniformf("cDValue", vcdc->GetElement(3, 2));
      cellBO.Program->SetUniformf("cSValue", -0.001 * diag * offset);
    }
  }

  // If the VBO coordinates were shifted and scaled, apply the inverse transform
  // to the model->view matrix:
  vtkOpenGLVertexBufferObject* vvbo = this->VBOs->GetVBO("vertexMC");
  if (vvbo && vvbo->GetCoordShiftAndScaleEnabled())
  {
    if (!actor->GetIsIdentity())
    {
      vtkMatrix4x4* mcwc;
      vtkMatrix3x3* anorms;
      static_cast<vtkOpenGLActor*>(actor)->GetKeyMatrices(mcwc, anorms);
      vtkMatrix4x4::Multiply4x4(this->VBOShiftScale, mcwc, this->TempMatrix4);
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
  {
    if (!actor->GetIsIdentity())
    {
      vtkMatrix4x4* mcwc;
      vtkMatrix3x3* anorms;
      ((vtkOpenGLActor*)actor)->GetKeyMatrices(mcwc, anorms);
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

  if (program->IsUniformUsed("cameraParallel"))
  {
    program->SetUniformi("cameraParallel", cam->GetParallelProjection());
  }
}

//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::SetPropertyShaderParameters(
  vtkOpenGLHelper& cellBO, vtkRenderer*, vtkActor* actor)
{
  vtkShaderProgram* program = cellBO.Program;

  vtkProperty* ppty = actor->GetProperty();

  {
    // Query the property for some of the properties that can be applied.
    float opacity = static_cast<float>(ppty->GetOpacity());
    double* aColor = this->DrawingEdgesOrVertices ? ppty->GetEdgeColor() : ppty->GetAmbientColor();
    aColor = cellBO.PrimitiveType == PrimitiveVertices ? ppty->GetVertexColor() : aColor;
    double aIntensity =
      (this->DrawingEdgesOrVertices && !this->DrawingTubesOrSpheres(cellBO, actor))
      ? 1.0
      : ppty->GetAmbient();

    double* dColor = this->DrawingEdgesOrVertices ? ppty->GetEdgeColor() : ppty->GetDiffuseColor();
    dColor = cellBO.PrimitiveType == PrimitiveVertices ? ppty->GetVertexColor() : dColor;
    double dIntensity =
      (this->DrawingEdgesOrVertices && !this->DrawingTubesOrSpheres(cellBO, actor))
      ? 0.0
      : ppty->GetDiffuse();

    double* sColor = ppty->GetSpecularColor();
    double sIntensity = (this->DrawingEdgesOrVertices && !this->DrawingTubes(cellBO, actor))
      ? 0.0
      : ppty->GetSpecular();
    double specularPower = ppty->GetSpecularPower();

    // these are always set
    program->SetUniformf("opacityUniform", opacity);
    program->SetUniformf("ambientIntensity", aIntensity);
    program->SetUniformf("diffuseIntensity", dIntensity);
    program->SetUniform3f("ambientColorUniform", aColor);
    program->SetUniform3f("diffuseColorUniform", dColor);

    if (this->VBOs->GetNumberOfComponents("tangentMC") == 3)
    {
      program->SetUniformf("normalScaleUniform", static_cast<float>(ppty->GetNormalScale()));
    }

    if (actor->GetProperty()->GetInterpolation() == VTK_PBR &&
      this->LastLightComplexity[this->LastBoundBO] > 0)
    {
      program->SetUniformf("metallicUniform", static_cast<float>(ppty->GetMetallic()));
      program->SetUniformf("roughnessUniform", static_cast<float>(ppty->GetRoughness()));
      program->SetUniformf("aoStrengthUniform", static_cast<float>(ppty->GetOcclusionStrength()));
      program->SetUniform3f("emissiveFactorUniform", ppty->GetEmissiveFactor());
    }

    // handle specular
    if (this->LastLightComplexity[&cellBO])
    {
      program->SetUniformf("specularIntensity", sIntensity);
      program->SetUniform3f("specularColorUniform", sColor);
      program->SetUniformf("specularPowerUniform", specularPower);
    }
  }

  // now set the backface properties if we have them
  if (program->IsUniformUsed("ambientIntensityBF"))
  {
    ppty = actor->GetBackfaceProperty();

    float opacity = static_cast<float>(ppty->GetOpacity());
    double* aColor = ppty->GetAmbientColor();
    double aIntensity = ppty->GetAmbient(); // ignoring renderer ambient
    double* dColor = ppty->GetDiffuseColor();
    double dIntensity = ppty->GetDiffuse();
    double* sColor = ppty->GetSpecularColor();
    double sIntensity = ppty->GetSpecular();
    double specularPower = ppty->GetSpecularPower();

    program->SetUniformf("ambientIntensityBF", aIntensity);
    program->SetUniformf("diffuseIntensityBF", dIntensity);
    program->SetUniformf("opacityUniformBF", opacity);
    program->SetUniform3f("ambientColorUniformBF", aColor);
    program->SetUniform3f("diffuseColorUniformBF", dColor);

    // handle specular
    if (this->LastLightComplexity[&cellBO])
    {
      program->SetUniformf("specularIntensityBF", sIntensity);
      program->SetUniform3f("specularColorUniformBF", sColor);
      program->SetUniformf("specularPowerUniformBF", specularPower);
    }
  }
}

namespace
{
// helper to get the state of picking
int getPickState(vtkRenderer* ren)
{
  vtkHardwareSelector* selector = ren->GetSelector();
  if (selector)
  {
    return selector->GetCurrentPass();
  }

  return vtkHardwareSelector::MIN_KNOWN_PASS - 1;
}
}

void vtkOpenGLPolyDataMapper::GetCoincidentParameters(
  vtkRenderer* ren, vtkActor* actor, float& factor, float& offset)
{
  // 1. ResolveCoincidentTopology is On and non zero for this primitive
  // type
  factor = 0.0;
  offset = 0.0;
  int primType = this->LastBoundBO->PrimitiveType;
  if (this->GetResolveCoincidentTopology() == VTK_RESOLVE_SHIFT_ZBUFFER &&
    (primType == PrimitiveTris || primType == PrimitiveTriStrips))
  {
    // do something rough is better than nothing
    double zRes = this->GetResolveCoincidentTopologyZShift(); // 0 is no shift 1 is big shift
    double f = zRes * 4.0;
    offset = f;
  }

  vtkProperty* prop = actor->GetProperty();
  if ((this->GetResolveCoincidentTopology() == VTK_RESOLVE_POLYGON_OFFSET) ||
    (prop->GetEdgeVisibility() && prop->GetRepresentation() == VTK_SURFACE))
  {
    double f = 0.0;
    double u = 0.0;
    if (primType == PrimitivePoints || prop->GetRepresentation() == VTK_POINTS)
    {
      this->GetCoincidentTopologyPointOffsetParameter(u);
    }
    else if (primType == PrimitiveLines || prop->GetRepresentation() == VTK_WIREFRAME)
    {
      this->GetCoincidentTopologyLineOffsetParameters(f, u);
    }
    else if (primType == PrimitiveTris || primType == PrimitiveTriStrips)
    {
      this->GetCoincidentTopologyPolygonOffsetParameters(f, u);
    }
    if (primType == PrimitiveTrisEdges || primType == PrimitiveTriStripsEdges)
    {
      this->GetCoincidentTopologyLineOffsetParameters(f, u);
    }
    factor = f;
    offset = u;
  }

  // hardware picking always offset due to saved zbuffer
  // This gets you above the saved surface depth buffer.
  vtkHardwareSelector* selector = ren->GetSelector();
  if (selector && selector->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    offset -= 2.0;
  }
}

void vtkOpenGLPolyDataMapper::UpdateMaximumPointCellIds(vtkRenderer* ren, vtkActor* actor)
{
  vtkHardwareSelector* selector = ren->GetSelector();

  // our maximum point id is the is the index of the max of
  // 1) the maximum used value in our points array
  // 2) the largest used value in a provided pointIdArray
  // To make this quicker we use the number of points for (1)
  // and the max range for (2)
  vtkIdType maxPointId = this->CurrentInput->GetPoints()->GetNumberOfPoints() - 1;
  if (this->CurrentInput && this->CurrentInput->GetPointData())
  {
    vtkIdTypeArray* pointArrayId = this->PointIdArrayName
      ? vtkArrayDownCast<vtkIdTypeArray>(
          this->CurrentInput->GetPointData()->GetArray(this->PointIdArrayName))
      : nullptr;
    if (pointArrayId)
    {
      maxPointId =
        maxPointId < pointArrayId->GetRange()[1] ? pointArrayId->GetRange()[1] : maxPointId;
    }
  }
  selector->UpdateMaximumPointId(maxPointId);

  bool pointPicking = false;
  if (selector && selector->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    pointPicking = true;
  }

  // the maximum number of cells in a draw call is the max of
  // 1) the sum of IBO size divided by the stride
  // 2) the max of any used call in a cellIdArray
  vtkIdType maxCellId = 0;
  int representation = actor->GetProperty()->GetRepresentation();
  for (int i = PrimitiveStart; i < PrimitiveTriStrips + 1; i++)
  {
    if (this->Primitives[i].IBO->IndexCount)
    {
      GLenum mode = this->GetOpenGLMode(representation, i);
      if (pointPicking)
      {
        mode = GL_POINTS;
      }
      unsigned int stride = (mode == GL_POINTS ? 1 : (mode == GL_LINES ? 2 : 3));
      vtkIdType strideMax = static_cast<vtkIdType>(this->Primitives[i].IBO->IndexCount / stride);
      maxCellId += strideMax;
    }
  }

  if (this->CurrentInput && this->CurrentInput->GetCellData())
  {
    vtkIdTypeArray* cellArrayId = this->CellIdArrayName
      ? vtkArrayDownCast<vtkIdTypeArray>(
          this->CurrentInput->GetCellData()->GetArray(this->CellIdArrayName))
      : nullptr;
    if (cellArrayId)
    {
      maxCellId = maxCellId < cellArrayId->GetRange()[1] ? cellArrayId->GetRange()[1] : maxCellId;
    }
  }
  selector->UpdateMaximumCellId(maxCellId);
}

//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::RenderPieceStart(vtkRenderer* ren, vtkActor* actor)
{
  // Set the PointSize and LineWidget
#ifndef GL_ES_VERSION_3_0
  glPointSize(actor->GetProperty()->GetPointSize()); // not on ES2
#endif

  // timer calls take time, for lots of "small" actors
  // the timer can be a big hit. So we only update
  // once per million cells or every 100 renders
  // whichever happens first
  vtkIdType numCells = this->CurrentInput->GetNumberOfCells();
  if (numCells != 0)
  {
    this->TimerQueryCounter++;
    if (this->TimerQueryCounter > 100 ||
      static_cast<double>(this->TimerQueryCounter) > 1000000.0 / numCells)
    {
      this->TimerQuery->ReusableStart();
      this->TimerQueryCounter = 0;
    }
  }

  vtkHardwareSelector* selector = ren->GetSelector();
  int picking = getPickState(ren);
  if (this->LastSelectionState != picking)
  {
    this->SelectionStateChanged.Modified();
    this->LastSelectionState = picking;
  }

  this->PrimitiveIDOffset = 0;

  // make sure the BOs are up to date
  this->UpdateBufferObjects(ren, actor);

  // render points for point picking in a special way
  if (selector && selector->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    static_cast<vtkOpenGLRenderer*>(ren)->GetState()->vtkglDepthMask(GL_FALSE);
  }
  if (selector && this->PopulateSelectionSettings)
  {
    selector->BeginRenderProp();
    if (selector->GetCurrentPass() == vtkHardwareSelector::COMPOSITE_INDEX_PASS)
    {
      selector->RenderCompositeIndex(1);
    }

    this->UpdateMaximumPointCellIds(ren, actor);
  }

  if (this->HaveCellScalars)
  {
    this->CellScalarTexture->Activate();
  }
  if (this->HaveCellNormals)
  {
    this->CellNormalTexture->Activate();
  }

  // If we are coloring by texture, then load the texture map.
  // Use Map as indicator, because texture hangs around.
  if (this->ColorTextureMap)
  {
    this->InternalColorTexture->Load(ren);
  }

  this->LastBoundBO = nullptr;
}

//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::RenderPieceDraw(vtkRenderer* ren, vtkActor* actor)
{
  int representation = actor->GetProperty()->GetRepresentation();

  // render points for point picking in a special way
  // all cell types should be rendered as points
  vtkHardwareSelector* selector = ren->GetSelector();
  bool pointPicking = false;
  if (selector && selector->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    pointPicking = true;
  }

#ifndef GL_ES_VERSION_3_0
  // when using IBL, we need seamless cubemaps to avoid artifacts
  if (ren->GetUseImageBasedLighting() && ren->GetEnvironmentCubeMap())
  {
    vtkOpenGLRenderWindow* renWin = vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());
    vtkOpenGLState* ostate = renWin->GetState();
    ostate->vtkglEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
  }
#endif

  bool draw_surface_with_edges =
    (actor->GetProperty()->GetEdgeVisibility() && representation == VTK_SURFACE) && !selector;
  int numVerts = this->VBOs->GetNumberOfTuples("vertexMC");
  for (int i = PrimitiveStart;
       i < (draw_surface_with_edges ? PrimitiveEnd : PrimitiveTriStrips + 1); i++)
  {
    this->DrawingEdgesOrVertices = (i > PrimitiveTriStrips ? true : false);
    if (this->Primitives[i].IBO->IndexCount)
    {
      GLenum mode = this->GetOpenGLMode(representation, i);
      if (pointPicking)
      {
#ifndef GL_ES_VERSION_3_0
        glPointSize(this->GetPointPickingPrimitiveSize(i));
#endif
        mode = GL_POINTS;
      }

      // Update/build/etc the shader.
      this->UpdateShaders(this->Primitives[i], ren, actor);

      if (mode == GL_LINES && !this->HaveWideLines(ren, actor))
      {
        glLineWidth(actor->GetProperty()->GetLineWidth());
      }

      this->Primitives[i].IBO->Bind();
      glDrawRangeElements(mode, 0, static_cast<GLuint>(numVerts - 1),
        static_cast<GLsizei>(this->Primitives[i].IBO->IndexCount), GL_UNSIGNED_INT, nullptr);
      this->Primitives[i].IBO->Release();
      if (i < 3)
      {
        this->PrimitiveIDOffset = this->CellCellMap->GetPrimitiveOffsets()[i + 1];
      }
    }
  }
}

//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::RenderPieceFinish(vtkRenderer* ren, vtkActor*)
{
  vtkHardwareSelector* selector = ren->GetSelector();
  // render points for point picking in a special way
  if (selector && selector->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    static_cast<vtkOpenGLRenderer*>(ren)->GetState()->vtkglDepthMask(GL_TRUE);
  }
  if (selector && this->PopulateSelectionSettings)
  {
    selector->EndRenderProp();
  }

  if (this->LastBoundBO)
  {
    this->LastBoundBO->VAO->Release();
  }

  if (this->ColorTextureMap)
  {
    this->InternalColorTexture->PostRender(ren);
  }

  // timer calls take time, for lots of "small" actors
  // the timer can be a big hit. So we assume zero time
  // for anything less than 100K cells
  if (this->TimerQueryCounter == 0)
  {
    this->TimerQuery->ReusableStop();
    this->TimeToDraw = this->TimerQuery->GetReusableElapsedSeconds();
    // If the timer is not accurate enough, set it to a small
    // time so that it is not zero
    if (this->TimeToDraw == 0.0)
    {
      this->TimeToDraw = 0.0001;
    }
  }

  if (this->HaveCellScalars)
  {
    this->CellScalarTexture->Deactivate();
  }
  if (this->HaveCellNormals)
  {
    this->CellNormalTexture->Deactivate();
  }

  this->UpdateProgress(1.0);
}

//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::RenderPiece(vtkRenderer* ren, vtkActor* actor)
{
  // Make sure that we have been properly initialized.
  if (ren->GetRenderWindow()->CheckAbortStatus())
  {
    return;
  }

  this->ResourceCallback->RegisterGraphicsResources(
    static_cast<vtkOpenGLRenderWindow*>(ren->GetRenderWindow()));

  this->CurrentInput = this->GetInput();

  if (this->CurrentInput == nullptr)
  {
    vtkErrorMacro(<< "No input!");
    return;
  }

  this->InvokeEvent(vtkCommand::StartEvent, nullptr);
  if (!this->Static)
  {
    this->GetInputAlgorithm()->Update();
  }
  this->InvokeEvent(vtkCommand::EndEvent, nullptr);

  // if there are no points then we are done
  if (!this->CurrentInput->GetPoints())
  {
    return;
  }

  this->RenderPieceStart(ren, actor);
  this->RenderPieceDraw(ren, actor);
  this->RenderPieceFinish(ren, actor);
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
void vtkOpenGLPolyDataMapper::UpdateBufferObjects(vtkRenderer* ren, vtkActor* act)
{
  // Rebuild buffers if needed
  if (this->GetNeedToRebuildBufferObjects(ren, act))
  {
    this->BuildBufferObjects(ren, act);
  }
}

//-------------------------------------------------------------------------
bool vtkOpenGLPolyDataMapper::GetNeedToRebuildBufferObjects(
  vtkRenderer* vtkNotUsed(ren), vtkActor* act)
{
  // we use a state vector instead of just mtime because
  // we do not want to check the actor's mtime.  Actor
  // changes mtime every time it's position changes. But
  // changing an actor's position does not require us to
  // rebuild all the VBO/IBOs. So we only watch the mtime
  // of the property/texture. But if someone changes the
  // Property on an actor the mtime may actually go down
  // because the new property has an older mtime. So
  // we watch the actual mtime, to see if it changes as
  // opposed to just checking if it is greater.
  this->TempState.Clear();
  this->TempState.Append(act->GetProperty()->GetMTime(), "actor mtime");
  this->TempState.Append(this->CurrentInput ? this->CurrentInput->GetMTime() : 0, "input mtime");
  this->TempState.Append(act->GetTexture() ? act->GetTexture()->GetMTime() : 0, "texture mtime");

  if (this->VBOBuildState != this->TempState || this->VBOBuildTime < this->GetMTime())
  {
    this->VBOBuildState = this->TempState;
    return true;
  }

  return false;
}

// create the cell scalar array adjusted for ogl Cells
void vtkOpenGLPolyDataMapper::AppendCellTextures(vtkRenderer* /*ren*/, vtkActor*,
  vtkCellArray* prims[4], int representation, std::vector<unsigned char>& newColors,
  std::vector<float>& newNorms, vtkPolyData* poly, vtkOpenGLCellToVTKCellMap* ccmap)
{
  vtkPoints* points = poly->GetPoints();

  if (this->HaveCellScalars || this->HaveCellNormals)
  {
    ccmap->Update(prims, representation, points);

    if (this->HaveCellScalars)
    {
      int numComp = this->Colors->GetNumberOfComponents();
      unsigned char* colorPtr = this->Colors->GetPointer(0);
      assert(numComp == 4);
      newColors.reserve(numComp * ccmap->GetSize());
      // use a single color value?
      if (this->FieldDataTupleId > -1 && this->ScalarMode == VTK_SCALAR_MODE_USE_FIELD_DATA)
      {
        for (size_t i = 0; i < ccmap->GetSize(); i++)
        {
          for (int j = 0; j < numComp; j++)
          {
            newColors.push_back(colorPtr[this->FieldDataTupleId * numComp + j]);
          }
        }
      }
      else
      {
        for (size_t i = 0; i < ccmap->GetSize(); i++)
        {
          for (int j = 0; j < numComp; j++)
          {
            newColors.push_back(colorPtr[ccmap->GetValue(i) * numComp + j]);
          }
        }
      }
    }

    if (this->HaveCellNormals)
    {
      // create the cell scalar array adjusted for ogl Cells
      vtkDataArray* n = this->CurrentInput->GetCellData()->GetNormals();
      newNorms.reserve(4 * ccmap->GetSize());
      for (size_t i = 0; i < ccmap->GetSize(); i++)
      {
        // RGB32F requires a later version of OpenGL than 3.2
        // with 3.2 we know we have RGBA32F hence the extra value
        double* norms = n->GetTuple(ccmap->GetValue(i));
        newNorms.push_back(norms[0]);
        newNorms.push_back(norms[1]);
        newNorms.push_back(norms[2]);
        newNorms.push_back(0);
      }
    }
  }
}

void vtkOpenGLPolyDataMapper::BuildCellTextures(
  vtkRenderer* ren, vtkActor* actor, vtkCellArray* prims[4], int representation)
{
  // create the cell scalar array adjusted for ogl Cells
  std::vector<unsigned char> newColors;
  std::vector<float> newNorms;
  this->AppendCellTextures(
    ren, actor, prims, representation, newColors, newNorms, this->CurrentInput, this->CellCellMap);

  // allocate as needed
  if (this->HaveCellScalars)
  {
    if (!this->CellScalarTexture)
    {
      this->CellScalarTexture = vtkTextureObject::New();
      this->CellScalarBuffer = vtkOpenGLBufferObject::New();
      this->CellScalarBuffer->SetType(vtkOpenGLBufferObject::TextureBuffer);
    }
    this->CellScalarTexture->SetContext(static_cast<vtkOpenGLRenderWindow*>(ren->GetVTKWindow()));
    this->CellScalarBuffer->Upload(newColors, vtkOpenGLBufferObject::TextureBuffer);
    this->CellScalarTexture->CreateTextureBuffer(static_cast<unsigned int>(newColors.size() / 4), 4,
      VTK_UNSIGNED_CHAR, this->CellScalarBuffer);
  }

  if (this->HaveCellNormals)
  {
    if (!this->CellNormalTexture)
    {
      this->CellNormalTexture = vtkTextureObject::New();
      this->CellNormalBuffer = vtkOpenGLBufferObject::New();
      this->CellNormalBuffer->SetType(vtkOpenGLBufferObject::TextureBuffer);
    }
    this->CellNormalTexture->SetContext(static_cast<vtkOpenGLRenderWindow*>(ren->GetVTKWindow()));

    // do we have float texture support ?
    int ftex = static_cast<vtkOpenGLRenderWindow*>(ren->GetRenderWindow())
                 ->GetDefaultTextureInternalFormat(VTK_FLOAT, 4, false, true, false);

    if (ftex)
    {
      this->CellNormalBuffer->Upload(newNorms, vtkOpenGLBufferObject::TextureBuffer);
      this->CellNormalTexture->CreateTextureBuffer(
        static_cast<unsigned int>(newNorms.size() / 4), 4, VTK_FLOAT, this->CellNormalBuffer);
    }
    else
    {
      // have to convert to unsigned char if no float support
      std::vector<unsigned char> ucNewNorms;
      ucNewNorms.resize(newNorms.size());
      for (size_t i = 0; i < newNorms.size(); i++)
      {
        ucNewNorms[i] = 127.0 * (newNorms[i] + 1.0);
      }
      this->CellNormalBuffer->Upload(ucNewNorms, vtkOpenGLBufferObject::TextureBuffer);
      this->CellNormalTexture->CreateTextureBuffer(static_cast<unsigned int>(newNorms.size() / 4),
        4, VTK_UNSIGNED_CHAR, this->CellNormalBuffer);
    }
  }
}

//-------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::BuildBufferObjects(vtkRenderer* ren, vtkActor* act)
{
  vtkPolyData* poly = this->CurrentInput;

  if (poly == nullptr)
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
    if (this->InternalColorTexture == nullptr)
    {
      this->InternalColorTexture = vtkOpenGLTexture::New();
      this->InternalColorTexture->RepeatOff();
    }
    this->InternalColorTexture->SetInputData(this->ColorTextureMap);
  }

  this->HaveCellScalars = false;
  vtkDataArray* c = this->Colors;
  if (this->ScalarVisibility)
  {
    // We must figure out how the scalars should be mapped to the polydata.
    if ((this->ScalarMode == VTK_SCALAR_MODE_USE_CELL_DATA ||
          this->ScalarMode == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA ||
          this->ScalarMode == VTK_SCALAR_MODE_USE_FIELD_DATA ||
          !poly->GetPointData()->GetScalars()) &&
      this->ScalarMode != VTK_SCALAR_MODE_USE_POINT_FIELD_DATA && this->Colors &&
      this->Colors->GetNumberOfTuples() > 0)
    {
      this->HaveCellScalars = true;
      c = nullptr;
    }
  }

  this->HaveCellNormals = false;
  // Do we have cell normals?
  vtkDataArray* n = (act->GetProperty()->GetInterpolation() != VTK_FLAT)
    ? poly->GetPointData()->GetNormals()
    : nullptr;
  if (n == nullptr && poly->GetCellData()->GetNormals())
  {
    this->HaveCellNormals = true;
  }

  int representation = act->GetProperty()->GetRepresentation();
  int interpolation = act->GetProperty()->GetInterpolation();

  vtkCellArray* prims[4];
  prims[0] = poly->GetVerts();
  prims[1] = poly->GetLines();
  prims[2] = poly->GetPolys();
  prims[3] = poly->GetStrips();

  this->CellCellMap->SetStartOffset(0);

  // only rebuild what we need to
  // if the data or mapper or selection state changed
  // then rebuild the cell arrays
  this->TempState.Clear();
  this->TempState.Append(prims[0]->GetNumberOfCells() ? prims[0]->GetMTime() : 0, "prim0 mtime");
  this->TempState.Append(prims[1]->GetNumberOfCells() ? prims[1]->GetMTime() : 0, "prim1 mtime");
  this->TempState.Append(prims[2]->GetNumberOfCells() ? prims[2]->GetMTime() : 0, "prim2 mtime");
  this->TempState.Append(prims[3]->GetNumberOfCells() ? prims[3]->GetMTime() : 0, "prim3 mtime");
  this->TempState.Append(representation, "representation");
  this->TempState.Append(interpolation, "interpolation");
  this->TempState.Append(this->LastSelectionState, "last selection state");
  this->TempState.Append(poly->GetMTime(), "polydata mtime");
  this->TempState.Append(this->GetMTime(), "this mtime");
  if (this->CellTextureBuildState != this->TempState)
  {
    this->CellTextureBuildState = this->TempState;
    this->BuildCellTextures(ren, act, prims, representation);
  }

  // if we have offsets from the cell map then use them
  this->CellCellMap->BuildPrimitiveOffsetsIfNeeded(prims, representation, poly->GetPoints());

  // Set the texture if we are going to use texture
  // for coloring with a point attribute.
  vtkDataArray* tcoords = nullptr;
  if (this->HaveTCoords(poly))
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

  vtkOpenGLRenderWindow* renWin = vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());
  vtkOpenGLVertexBufferObjectCache* cache = renWin->GetVBOCache();

  // rebuild VBO if needed
  for (auto& itr : this->ExtraAttributes)
  {
    vtkDataArray* da = poly->GetPointData()->GetArray(itr.second.DataArrayName.c_str());
    this->VBOs->CacheDataArray(itr.first.c_str(), da, cache, VTK_FLOAT);
  }

  this->VBOs->CacheDataArray("vertexMC", poly->GetPoints()->GetData(), cache, VTK_FLOAT);
  vtkOpenGLVertexBufferObject* posVBO = this->VBOs->GetVBO("vertexMC");
  if (posVBO)
  {
    posVBO->SetCoordShiftAndScaleMethod(
      static_cast<vtkOpenGLVertexBufferObject::ShiftScaleMethod>(this->ShiftScaleMethod));
  }

  this->VBOs->CacheDataArray("normalMC", n, cache, VTK_FLOAT);
  this->VBOs->CacheDataArray("scalarColor", c, cache, VTK_UNSIGNED_CHAR);
  this->VBOs->CacheDataArray("tcoord", tcoords, cache, VTK_FLOAT);

  // Look for tangents attribute
  vtkFloatArray* tangents = vtkFloatArray::SafeDownCast(poly->GetPointData()->GetTangents());
  if (tangents)
  {
    this->VBOs->CacheDataArray("tangentMC", tangents, cache, VTK_FLOAT);
  }

  this->VBOs->BuildAllVBOs(cache);

  // get it again as it may have been freed
  posVBO = this->VBOs->GetVBO("vertexMC");
  if (posVBO && posVBO->GetCoordShiftAndScaleEnabled())
  {
    std::vector<double> shift = posVBO->GetShift();
    std::vector<double> scale = posVBO->GetScale();
    this->VBOInverseTransform->Identity();
    this->VBOInverseTransform->Translate(shift[0], shift[1], shift[2]);
    this->VBOInverseTransform->Scale(1.0 / scale[0], 1.0 / scale[1], 1.0 / scale[2]);
    this->VBOInverseTransform->GetTranspose(this->VBOShiftScale);
  }

  // now create the IBOs
  this->BuildIBO(ren, act, poly);

  vtkOpenGLCheckErrorMacro("failed after BuildBufferObjects");

  this->VBOBuildTime
    .Modified(); // need to call all the time or GetNeedToRebuild will always return true;
}

//-------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::BuildIBO(vtkRenderer* /* ren */, vtkActor* act, vtkPolyData* poly)
{
  vtkCellArray* prims[4];
  prims[0] = poly->GetVerts();
  prims[1] = poly->GetLines();
  prims[2] = poly->GetPolys();
  prims[3] = poly->GetStrips();
  int representation = act->GetProperty()->GetRepresentation();

  vtkDataArray* ef = poly->GetPointData()->GetAttribute(vtkDataSetAttributes::EDGEFLAG);
  vtkProperty* prop = act->GetProperty();

  bool draw_surface_with_edges =
    (prop->GetEdgeVisibility() && prop->GetRepresentation() == VTK_SURFACE);

  // do we really need to rebuild the IBO? Since the operation is costly we
  // construst a string of values that impact the IBO and see if that string has
  // changed

  // So...polydata can return a dummy CellArray when there are no lines
  this->TempState.Append(prims[0]->GetNumberOfCells() ? prims[0]->GetMTime() : 0, "prim0 mtime");
  this->TempState.Append(prims[1]->GetNumberOfCells() ? prims[1]->GetMTime() : 0, "prim1 mtime");
  this->TempState.Append(prims[2]->GetNumberOfCells() ? prims[2]->GetMTime() : 0, "prim2 mtime");
  this->TempState.Append(prims[3]->GetNumberOfCells() ? prims[3]->GetMTime() : 0, "prim3 mtime");
  this->TempState.Append(representation, "representation");
  this->TempState.Append(ef ? ef->GetMTime() : 0, "edge flags mtime");
  this->TempState.Append(draw_surface_with_edges, "draw surface with edges");

  if (this->IBOBuildState != this->TempState)
  {
    this->IBOBuildState = this->TempState;
    this->Primitives[PrimitivePoints].IBO->CreatePointIndexBuffer(prims[0]);

    if (representation == VTK_POINTS)
    {
      this->Primitives[PrimitiveLines].IBO->CreatePointIndexBuffer(prims[1]);
      this->Primitives[PrimitiveTris].IBO->CreatePointIndexBuffer(prims[2]);
      this->Primitives[PrimitiveTriStrips].IBO->CreatePointIndexBuffer(prims[3]);
    }
    else // WIREFRAME OR SURFACE
    {
      this->Primitives[PrimitiveLines].IBO->CreateLineIndexBuffer(prims[1]);

      if (representation == VTK_WIREFRAME)
      {
        if (ef)
        {
          if (ef->GetNumberOfComponents() != 1)
          {
            vtkDebugMacro(<< "Currently only 1d edge flags are supported.");
            ef = nullptr;
          }
          if (!ef->IsA("vtkUnsignedCharArray"))
          {
            vtkDebugMacro(<< "Currently only unsigned char edge flags are supported.");
            ef = nullptr;
          }
        }
        if (ef)
        {
          this->Primitives[PrimitiveTris].IBO->CreateEdgeFlagIndexBuffer(prims[2], ef);
        }
        else
        {
          this->Primitives[PrimitiveTris].IBO->CreateTriangleLineIndexBuffer(prims[2]);
        }
        this->Primitives[PrimitiveTriStrips].IBO->CreateStripIndexBuffer(prims[3], true);
      }
      else // SURFACE
      {
        this->Primitives[PrimitiveTris].IBO->CreateTriangleIndexBuffer(prims[2], poly->GetPoints());
        this->Primitives[PrimitiveTriStrips].IBO->CreateStripIndexBuffer(prims[3], false);
      }
    }

    // when drawing edges also build the edge IBOs
    if (draw_surface_with_edges)
    {
      if (ef)
      {
        if (ef->GetNumberOfComponents() != 1)
        {
          vtkDebugMacro(<< "Currently only 1d edge flags are supported.");
          ef = nullptr;
        }
        else if (!ef->IsA("vtkUnsignedCharArray"))
        {
          vtkDebugMacro(<< "Currently only unsigned char edge flags are supported.");
          ef = nullptr;
        }
      }
      if (ef)
      {
        this->Primitives[PrimitiveTrisEdges].IBO->CreateEdgeFlagIndexBuffer(prims[2], ef);
      }
      else
      {
        this->Primitives[PrimitiveTrisEdges].IBO->CreateTriangleLineIndexBuffer(prims[2]);
      }
      this->Primitives[PrimitiveTriStripsEdges].IBO->CreateStripIndexBuffer(prims[3], true);
    }

    if (prop->GetVertexVisibility())
    {
      // for all 4 types of primitives add their verts into the IBO
      this->Primitives[PrimitiveVertices].IBO->CreateVertexIndexBuffer(prims);
    }
  }
}

//----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::ShallowCopy(vtkAbstractMapper* mapper)
{
  vtkOpenGLPolyDataMapper* m = vtkOpenGLPolyDataMapper::SafeDownCast(mapper);
  if (m != nullptr)
  {
    this->SetPointIdArrayName(m->GetPointIdArrayName());
    this->SetCompositeIdArrayName(m->GetCompositeIdArrayName());
    this->SetProcessIdArrayName(m->GetProcessIdArrayName());
    this->SetCellIdArrayName(m->GetCellIdArrayName());
#ifndef VTK_LEGACY_REMOVE
    this->SetVertexShaderCode(m->GetVertexShaderCode());
    this->SetGeometryShaderCode(m->GetGeometryShaderCode());
    this->SetFragmentShaderCode(m->GetFragmentShaderCode());
#endif
  }

  // Now do superclass
  this->vtkPolyDataMapper::ShallowCopy(mapper);
}

void vtkOpenGLPolyDataMapper::SetVBOShiftScaleMethod(int m)
{
  this->ShiftScaleMethod = m;
}

int vtkOpenGLPolyDataMapper::GetOpenGLMode(int representation, int primType)
{
  if (representation == VTK_POINTS || primType == PrimitivePoints || primType == PrimitiveVertices)
  {
    return GL_POINTS;
  }
  if (representation == VTK_WIREFRAME || primType == PrimitiveLines ||
    primType == PrimitiveTrisEdges || primType == PrimitiveTriStripsEdges)
  {
    return GL_LINES;
  }
  return GL_TRIANGLES;
}

int vtkOpenGLPolyDataMapper::GetPointPickingPrimitiveSize(int primType)
{
  if (primType == PrimitivePoints)
  {
    return 2;
  }
  if (primType == PrimitiveLines)
  {
    return 4;
  }
  return 6;
}

//----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::MapDataArrayToVertexAttribute(
  const char* vertexAttributeName, const char* dataArrayName, int fieldAssociation, int componentno)
{
  this->MapDataArray(vertexAttributeName, dataArrayName, "", fieldAssociation, componentno);
}

void vtkOpenGLPolyDataMapper::MapDataArrayToMultiTextureAttribute(
  const char* tname, const char* dataArrayName, int fieldAssociation, int componentno)
{
  std::string coordname = tname;
  coordname += "_coord";
  this->MapDataArray(coordname.c_str(), dataArrayName, tname, fieldAssociation, componentno);
}

void vtkOpenGLPolyDataMapper::MapDataArray(const char* vertexAttributeName,
  const char* dataArrayName, const char* tname, int fieldAssociation, int componentno)
{
  if (!vertexAttributeName)
  {
    return;
  }

  // store the mapping in the map
  this->RemoveVertexAttributeMapping(vertexAttributeName);
  if (!dataArrayName)
  {
    return;
  }

  vtkOpenGLPolyDataMapper::ExtraAttributeValue aval;
  aval.DataArrayName = dataArrayName;
  aval.FieldAssociation = fieldAssociation;
  aval.ComponentNumber = componentno;
  aval.TextureName = tname;

  this->ExtraAttributes.insert(std::make_pair(vertexAttributeName, aval));

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::RemoveVertexAttributeMapping(const char* vertexAttributeName)
{
  auto itr = this->ExtraAttributes.find(vertexAttributeName);
  if (itr != this->ExtraAttributes.end())
  {
    this->VBOs->RemoveAttribute(vertexAttributeName);
    this->ExtraAttributes.erase(itr);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::RemoveAllVertexAttributeMappings()
{
  for (auto itr = this->ExtraAttributes.begin(); itr != this->ExtraAttributes.end();
       itr = this->ExtraAttributes.begin())
  {
    this->RemoveVertexAttributeMapping(itr->first.c_str());
  }
}

//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void vtkOpenGLPolyDataMapper::ProcessSelectorPixelBuffers(
  vtkHardwareSelector* sel, std::vector<unsigned int>& pixeloffsets, vtkProp* prop)
{
  vtkPolyData* poly = this->CurrentInput;

  if (!this->PopulateSelectionSettings || !poly)
  {
    return;
  }

  // which pass are we processing ?
  int currPass = sel->GetCurrentPass();

  // get some common useful values
  bool pointPicking = sel->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_POINTS;
  vtkPointData* pd = poly->GetPointData();
  vtkCellData* cd = poly->GetCellData();
  unsigned char* rawplowdata = sel->GetRawPixelBuffer(vtkHardwareSelector::POINT_ID_LOW24);
  unsigned char* rawphighdata = sel->GetRawPixelBuffer(vtkHardwareSelector::POINT_ID_HIGH24);

  // handle process pass
  if (currPass == vtkHardwareSelector::PROCESS_PASS)
  {
    vtkUnsignedIntArray* processArray = nullptr;

    // point data is used for process_pass which seems odd
    if (sel->GetUseProcessIdFromData())
    {
      processArray = this->ProcessIdArrayName
        ? vtkArrayDownCast<vtkUnsignedIntArray>(pd->GetArray(this->ProcessIdArrayName))
        : nullptr;
    }

    // do we need to do anything to the process pass data?
    unsigned char* processdata = sel->GetRawPixelBuffer(vtkHardwareSelector::PROCESS_PASS);
    if (processArray && processdata && rawplowdata)
    {
      // get the buffer pointers we need
      for (auto pos : pixeloffsets)
      {
        unsigned int inval = 0;
        if (rawphighdata)
        {
          inval = rawphighdata[pos];
          inval = inval << 8;
        }
        inval |= rawplowdata[pos + 2];
        inval = inval << 8;
        inval |= rawplowdata[pos + 1];
        inval = inval << 8;
        inval |= rawplowdata[pos];
        inval -= 1;
        unsigned int outval = processArray->GetValue(inval) + 1;
        processdata[pos] = outval & 0xff;
        processdata[pos + 1] = (outval & 0xff00) >> 8;
        processdata[pos + 2] = (outval & 0xff0000) >> 16;
      }
    }
  }

  if (currPass == vtkHardwareSelector::POINT_ID_LOW24)
  {
    vtkIdTypeArray* pointArrayId = this->PointIdArrayName
      ? vtkArrayDownCast<vtkIdTypeArray>(pd->GetArray(this->PointIdArrayName))
      : nullptr;

    // do we need to do anything to the point id data?
    if (rawplowdata && pointArrayId)
    {
      unsigned char* plowdata = sel->GetPixelBuffer(vtkHardwareSelector::POINT_ID_LOW24);

      for (auto pos : pixeloffsets)
      {
        unsigned int inval = 0;
        if (rawphighdata)
        {
          inval = rawphighdata[pos];
          inval = inval << 8;
        }
        inval |= rawplowdata[pos + 2];
        inval = inval << 8;
        inval |= rawplowdata[pos + 1];
        inval = inval << 8;
        inval |= rawplowdata[pos];
        inval -= 1;
        vtkIdType outval = pointArrayId->GetValue(inval) + 1;
        plowdata[pos] = outval & 0xff;
        plowdata[pos + 1] = (outval & 0xff00) >> 8;
        plowdata[pos + 2] = (outval & 0xff0000) >> 16;
      }
    }
  }

  if (currPass == vtkHardwareSelector::POINT_ID_HIGH24)
  {
    vtkIdTypeArray* pointArrayId = this->PointIdArrayName
      ? vtkArrayDownCast<vtkIdTypeArray>(pd->GetArray(this->PointIdArrayName))
      : nullptr;

    // do we need to do anything to the point id data?
    if (rawphighdata && pointArrayId)
    {
      unsigned char* phighdata = sel->GetPixelBuffer(vtkHardwareSelector::POINT_ID_HIGH24);

      for (auto pos : pixeloffsets)
      {
        unsigned int inval = 0;
        inval = rawphighdata[pos];
        inval = inval << 8;
        inval |= rawplowdata[pos + 2];
        inval = inval << 8;
        inval |= rawplowdata[pos + 1];
        inval = inval << 8;
        inval |= rawplowdata[pos];
        inval -= 1;
        vtkIdType outval = pointArrayId->GetValue(inval) + 1;
        phighdata[pos] = (outval & 0xff000000) >> 24;
        phighdata[pos + 1] = (outval & 0xff00000000) >> 32;
        phighdata[pos + 2] = (outval & 0xff0000000000) >> 40;
      }
    }
  }

  // vars for cell based indexing
  vtkCellArray* prims[4];
  prims[0] = poly->GetVerts();
  prims[1] = poly->GetLines();
  prims[2] = poly->GetPolys();
  prims[3] = poly->GetStrips();

  int representation = static_cast<vtkActor*>(prop)->GetProperty()->GetRepresentation();

  unsigned char* rawclowdata = sel->GetRawPixelBuffer(vtkHardwareSelector::CELL_ID_LOW24);
  unsigned char* rawchighdata = sel->GetRawPixelBuffer(vtkHardwareSelector::CELL_ID_HIGH24);

  // do we need to do anything to the composite pass data?
  if (currPass == vtkHardwareSelector::COMPOSITE_INDEX_PASS)
  {
    unsigned char* compositedata = sel->GetPixelBuffer(vtkHardwareSelector::COMPOSITE_INDEX_PASS);

    vtkUnsignedIntArray* compositeArray = this->CompositeIdArrayName
      ? vtkArrayDownCast<vtkUnsignedIntArray>(cd->GetArray(this->CompositeIdArrayName))
      : nullptr;

    if (compositedata && compositeArray && rawclowdata)
    {
      this->CellCellMap->Update(prims, representation, poly->GetPoints());

      for (auto pos : pixeloffsets)
      {
        unsigned int inval = 0;
        if (rawchighdata)
        {
          inval = rawchighdata[pos];
          inval = inval << 8;
        }
        inval |= rawclowdata[pos + 2];
        inval = inval << 8;
        inval |= rawclowdata[pos + 1];
        inval = inval << 8;
        inval |= rawclowdata[pos];
        inval -= 1;
        vtkIdType vtkCellId =
          this->CellCellMap->ConvertOpenGLCellIdToVTKCellId(pointPicking, inval);
        unsigned int outval = compositeArray->GetValue(vtkCellId) + 1;
        compositedata[pos] = outval & 0xff;
        compositedata[pos + 1] = (outval & 0xff00) >> 8;
        compositedata[pos + 2] = (outval & 0xff0000) >> 16;
      }
    }
  }

  // process the cellid array?
  if (currPass == vtkHardwareSelector::CELL_ID_LOW24)
  {
    vtkIdTypeArray* cellArrayId = this->CellIdArrayName
      ? vtkArrayDownCast<vtkIdTypeArray>(cd->GetArray(this->CellIdArrayName))
      : nullptr;
    unsigned char* clowdata = sel->GetPixelBuffer(vtkHardwareSelector::CELL_ID_LOW24);

    if (rawclowdata)
    {
      this->CellCellMap->Update(prims, representation, poly->GetPoints());

      for (auto pos : pixeloffsets)
      {
        unsigned int inval = 0;
        if (rawchighdata)
        {
          inval = rawchighdata[pos];
          inval = inval << 8;
        }
        inval |= rawclowdata[pos + 2];
        inval = inval << 8;
        inval |= rawclowdata[pos + 1];
        inval = inval << 8;
        inval |= rawclowdata[pos];
        inval -= 1;
        vtkIdType outval = this->CellCellMap->ConvertOpenGLCellIdToVTKCellId(pointPicking, inval);
        if (cellArrayId)
        {
          outval = cellArrayId->GetValue(outval);
        }
        outval++;
        clowdata[pos] = outval & 0xff;
        clowdata[pos + 1] = (outval & 0xff00) >> 8;
        clowdata[pos + 2] = (outval & 0xff0000) >> 16;
      }
    }
  }

  if (currPass == vtkHardwareSelector::CELL_ID_HIGH24)
  {
    vtkIdTypeArray* cellArrayId = this->CellIdArrayName
      ? vtkArrayDownCast<vtkIdTypeArray>(cd->GetArray(this->CellIdArrayName))
      : nullptr;
    unsigned char* chighdata = sel->GetPixelBuffer(vtkHardwareSelector::CELL_ID_HIGH24);

    if (rawchighdata)
    {
      this->CellCellMap->Update(prims, representation, poly->GetPoints());

      for (auto pos : pixeloffsets)
      {
        unsigned int inval = 0;
        inval = rawchighdata[pos];
        inval = inval << 8;
        inval |= rawclowdata[pos + 2];
        inval = inval << 8;
        inval |= rawclowdata[pos + 1];
        inval = inval << 8;
        inval |= rawclowdata[pos];
        inval -= 1;
        vtkIdType outval = this->CellCellMap->ConvertOpenGLCellIdToVTKCellId(pointPicking, inval);
        if (cellArrayId)
        {
          outval = cellArrayId->GetValue(outval);
        }
        outval++;
        chighdata[pos] = (outval & 0xff000000) >> 24;
        chighdata[pos + 1] = (outval & 0xff00000000) >> 32;
        chighdata[pos + 2] = (outval & 0xff0000000000) >> 40;
      }
    }
  }
}
