/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Hide VTK_DEPRECATED_IN_9_0_0() warnings for this class.
#define VTK_DEPRECATION_LEVEL 0

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
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkShaderProgram.h"
#include "vtkTextureObject.h"
#include "vtkTransform.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"

#include "vtkPBRFunctions.h"
// Bring in our fragment lit shader symbols.
#include "vtkPolyDataEdgesGS.h"
#include "vtkPolyDataFS.h"
#include "vtkPolyDataVS.h"
#include "vtkPolyDataWideLineGS.h"

#include <algorithm>
#include <sstream>

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLPolyDataMapper);

//------------------------------------------------------------------------------
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
  this->DrawingVertices = false;
  this->ForceTextureCoordinates = false;
  this->SelectionType = VTK_POINTS;

  this->PrimitiveIDOffset = 0;
  this->ShiftScaleMethod = vtkOpenGLVertexBufferObject::AUTO_SHIFT_SCALE;

  this->CellScalarTexture = nullptr;
  this->CellScalarBuffer = nullptr;
  this->CellNormalTexture = nullptr;
  this->CellNormalBuffer = nullptr;

  this->EdgeTexture = nullptr;
  this->EdgeBuffer = nullptr;

  this->HaveCellScalars = false;
  this->HaveCellNormals = false;

  this->PointIdArrayName = nullptr;
  this->CellIdArrayName = nullptr;
  this->ProcessIdArrayName = nullptr;
  this->CompositeIdArrayName = nullptr;
  this->VBOs = vtkOpenGLVertexBufferObjectGroup::New();

  this->LastBoundBO = nullptr;

  for (int i = vtkOpenGLPolyDataMapper::PrimitiveStart; i < vtkOpenGLPolyDataMapper::PrimitiveEnd;
       i++)
  {
    this->LastLightComplexity[&this->Primitives[i]] = -1;
    this->LastLightCount[&this->Primitives[i]] = 0;
    this->Primitives[i].PrimitiveType = i;
    this->SelectionPrimitives[i].PrimitiveType = i;
  }

  this->ResourceCallback = new vtkOpenGLResourceFreeCallback<vtkOpenGLPolyDataMapper>(
    this, &vtkOpenGLPolyDataMapper::ReleaseGraphicsResources);

  // initialize to 1 as 0 indicates we have initiated a request
  this->TimerQueryCounter = 1;
  this->TimeToDraw = 0.0001;
}

//------------------------------------------------------------------------------
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

  if (this->EdgeTexture)
  { // Resources released previously.
    this->EdgeTexture->Delete();
    this->EdgeTexture = nullptr;
  }
  if (this->EdgeBuffer)
  { // Resources released previously.
    this->EdgeBuffer->Delete();
    this->EdgeBuffer = nullptr;
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

//------------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::ReleaseGraphicsResources(vtkWindow* win)
{
  if (!this->ResourceCallback->IsReleasing())
  {
    this->ResourceCallback->Release();
    return;
  }

  this->VBOs->ReleaseGraphicsResources(win);
  for (int i = vtkOpenGLPolyDataMapper::PrimitiveStart; i < vtkOpenGLPolyDataMapper::PrimitiveEnd;
       i++)
  {
    this->Primitives[i].ReleaseGraphicsResources(win);
    this->SelectionPrimitives[i].ReleaseGraphicsResources(win);
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
  if (this->EdgeTexture)
  {
    this->EdgeTexture->ReleaseGraphicsResources(win);
  }
  if (this->EdgeBuffer)
  {
    this->EdgeBuffer->ReleaseGraphicsResources();
  }
  this->TimerQuery->ReleaseGraphicsResources();
  this->VBOBuildState.Clear();
  this->IBOBuildState.Clear();
  this->CellTextureBuildState.Clear();
  this->Modified();
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::ClearShaderReplacement(
  vtkShader::Type shaderType, // vertex, fragment, etc
  const std::string& originalValue, bool replaceFirst)
{
  VTK_LEGACY_REPLACED_BODY(vtkOpenGLPolyDataMapper::ClearShaderReplacement, "VTK 9.0",
    vtkOpenGLShaderProperty::ClearShaderReplacement);
  this->GetLegacyShaderProperty()->ClearShaderReplacement(shaderType, originalValue, replaceFirst);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::ClearAllShaderReplacements(vtkShader::Type shaderType)
{
  VTK_LEGACY_REPLACED_BODY(vtkOpenGLPolyDataMapper::ClearAllShaderReplacements, "VTK 9.0",
    vtkOpenGLShaderProperty::ClearAllShaderReplacements);
  this->GetLegacyShaderProperty()->ClearAllShaderReplacements(shaderType);
  this->Modified();
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::BuildShaders(
  std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* actor)
{
  // in cases where LegacyShaderProperty is not nullptr, it means someone has used
  // legacy shader replacement functions, so we make sure the actor uses the same
  // shader property. NOTE: this implies that it is not possible to use both legacy
  // and new functionality on the same actor/mapper.
  if (this->LegacyShaderProperty && actor->GetShaderProperty() != this->LegacyShaderProperty)
  {
    actor->SetShaderProperty(this->LegacyShaderProperty);
  }

  this->GetShaderTemplate(shaders, ren, actor);

  // user specified pre replacements
  vtkOpenGLShaderProperty* sp = vtkOpenGLShaderProperty::SafeDownCast(actor->GetShaderProperty());
  vtkOpenGLShaderProperty::ReplacementMap repMap = sp->GetAllShaderReplacements();
  for (const auto& i : repMap)
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
  for (const auto& i : repMap)
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

//------------------------------------------------------------------------------
bool vtkOpenGLPolyDataMapper::HaveWideLines(vtkRenderer* ren, vtkActor* actor)
{
  vtkHardwareSelector* selector = ren->GetSelector();
  if (selector && selector->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    return false;
  }

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
  return this->DrawingSelection &&
    (this->GetOpenGLMode(this->SelectionType, this->LastBoundBO->PrimitiveType) == GL_LINES);
}

bool vtkOpenGLPolyDataMapper::DrawingEdges(vtkRenderer*, vtkActor* actor)
{
  if (this->PointPicking)
  {
    return false;
  }

  if (actor->GetProperty()->GetEdgeVisibility() &&
    this->GetOpenGLMode(
      actor->GetProperty()->GetRepresentation(), this->LastBoundBO->PrimitiveType) == GL_TRIANGLES)
  {
    return true;
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
  for (const auto& it : this->ExtraAttributes)
  {
    if (it.second.TextureName == tname)
    {
      return it.first;
    }
  }
  return std::string("tcoord");
}

//------------------------------------------------------------------------------
bool vtkOpenGLPolyDataMapper::HaveTextures(vtkActor* actor)
{
  return (this->GetNumberOfTextures(actor) > 0);
}

typedef std::pair<vtkTexture*, std::string> texinfo;

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
std::vector<texinfo> vtkOpenGLPolyDataMapper::GetTextures(vtkActor* actor)
{
  std::vector<texinfo> res;

  if (this->ColorTextureMap)
  {
    res.emplace_back(this->InternalColorTexture, "colortexture");
  }
  if (actor->GetTexture())
  {
    res.emplace_back(actor->GetTexture(), "actortexture");
  }
  auto textures = actor->GetProperty()->GetAllTextures();
  for (const auto& ti : textures)
  {
    res.emplace_back(ti.second, ti.first);
  }
  return res;
}

//------------------------------------------------------------------------------
bool vtkOpenGLPolyDataMapper::HaveTCoords(vtkPolyData* poly)
{
  return (
    this->ColorCoordinates || poly->GetPointData()->GetTCoords() || this->ForceTextureCoordinates);
}

//------------------------------------------------------------------------------
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
    if (this->DrawingEdges(ren, actor))
    {
      shaders[vtkShader::Geometry]->SetSource(vtkPolyDataEdgesGS);
    }
    else if (this->HaveWideLines(ren, actor))
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
void vtkOpenGLPolyDataMapper::ReplaceShaderEdges(
  std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* actor)
{
  if (this->DrawingEdges(ren, actor))
  {
    if (this->LastBoundBO->PrimitiveType == PrimitiveTris)
    {
      std::string GSSource = shaders[vtkShader::Geometry]->GetSource();

      if (this->EdgeValues.size())
      {
        vtkShaderProgram::Substitute(
          GSSource, "//VTK::Edges::Dec", "uniform samplerBuffer edgeTexture;");
        vtkShaderProgram::Substitute(GSSource, "//VTK::Edges::Impl",
          "float edgeValues = 255.0*texelFetch(edgeTexture, gl_PrimitiveIDIn + "
          "PrimitiveIDOffset).r;\n"
          "if (edgeValues < 4.0) edgeEqn[2].z = lineWidth;\n"
          "if (mod(edgeValues, 4.0) < 2.0) edgeEqn[1].z = lineWidth;\n"
          "if (mod(edgeValues, 2.0) < 1.0) edgeEqn[0].z = lineWidth;\n");
      }
      shaders[vtkShader::Geometry]->SetSource(GSSource);
    }

    // discard pixels that are outside the polygon and not an edge

    std::string FSSource = shaders[vtkShader::Fragment]->GetSource();

    vtkShaderProgram::Substitute(FSSource, "//VTK::Edges::Dec",
      "in vec4 edgeEqn[3];\n"
      "uniform float lineWidth;\n"
      "uniform vec3 edgeColor;\n");

    std::string fsimpl =
      // distance gets larger as you go inside the polygon
      "float edist[3];\n"
      "edist[0] = dot(edgeEqn[0].xy, gl_FragCoord.xy) + edgeEqn[0].w;\n"
      "edist[1] = dot(edgeEqn[1].xy, gl_FragCoord.xy) + edgeEqn[1].w;\n"
      "edist[2] = dot(edgeEqn[2].xy, gl_FragCoord.xy) + edgeEqn[2].w;\n"

      // "if (abs(edist[0]) > 0.5*lineWidth && abs(edist[1]) > 0.5*lineWidth && abs(edist[2]) >
      // 0.5*lineWidth) discard;\n"

      "if (edist[0] < -0.5 && edgeEqn[0].z > 0.0) discard;\n"
      "if (edist[1] < -0.5 && edgeEqn[1].z > 0.0) discard;\n"
      "if (edist[2] < -0.5 && edgeEqn[2].z > 0.0) discard;\n"

      "edist[0] += edgeEqn[0].z;\n"
      "edist[1] += edgeEqn[1].z;\n"
      "edist[2] += edgeEqn[2].z;\n"

      "float emix = clamp(0.5 + 0.5*lineWidth - min( min( edist[0], edist[1]), edist[2]), 0.0, "
      "1.0);\n";

    if (actor->GetProperty()->GetRenderLinesAsTubes())
    {
      fsimpl += "  diffuseColor = mix(diffuseColor, diffuseIntensity*edgeColor, emix);\n"
                "  ambientColor = mix(ambientColor, ambientIntensity*edgeColor, emix);\n"
        // " else { discard; }\n" // this yields wireframe only
        ;
    }
    else
    {
      fsimpl += "  diffuseColor = mix(diffuseColor, vec3(0.0), emix);\n"
                "  ambientColor = mix( ambientColor, edgeColor, emix);\n"
        // " else { discard; }\n" // this yields wireframe only
        ;
    }
    vtkShaderProgram::Substitute(FSSource, "//VTK::Edges::Impl", fsimpl);

    // even more fake tubes, for surface with edges this implementation
    // just adjusts the normal calculation but not the zbuffer
    if (actor->GetProperty()->GetRenderLinesAsTubes())
    {
      vtkShaderProgram::Substitute(FSSource, "//VTK::Normal::Impl",
        "//VTK::Normal::Impl\n"
        "  float cdist = min(edist[0], edist[1]);\n"
        "  vec4 cedge = mix(edgeEqn[0], edgeEqn[1], 0.5 + 0.5*sign(edist[0] - edist[1]));\n"
        "  cedge = mix(cedge, edgeEqn[2], 0.5 + 0.5*sign(cdist - edist[2]));\n"
        "  vec3 tnorm = normalize(cross(normalVCVSOutput, cross(vec3(cedge.xy,0.0), "
        "normalVCVSOutput)));\n"
        "  float rdist = 2.0*min(cdist, edist[2])/lineWidth;\n"

        // these two lines adjust for the fact that normally part of the
        // tube would be self occluded but as these are fake tubes this does
        // not happen. The code adjusts the computed location on the tube as
        // the surface normal dot view direction drops.
        "  float A = tnorm.z;\n"
        "  rdist = 0.5*rdist + 0.5*(rdist + A)/(1+abs(A));\n"

        "  float lenZ = clamp(sqrt(1.0 - rdist*rdist),0.0,1.0);\n"
        "  normalVCVSOutput = mix(normalVCVSOutput, normalize(rdist*tnorm + "
        "normalVCVSOutput*lenZ), emix);\n");
    }

    shaders[vtkShader::Fragment]->SetSource(FSSource);
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::ReplaceShaderColor(
  std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer*, vtkActor* actor)
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

  // handle color point attributes
  if (this->VBOs->GetNumberOfComponents("scalarColor") != 0 && !this->DrawingVertices)
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
    !this->DrawingVertices)
  {
    colorImpl += "  vec4 texColor = texture(colortexture, tcoordVCVSOutput.st);\n"
                 "  vec3 ambientColor = ambientIntensity * texColor.rgb;\n"
                 "  vec3 diffuseColor = diffuseIntensity * texColor.rgb;\n"
                 "  float opacity = opacityUniform * texColor.a;";
  }
  // are we doing cell scalar coloring by texture?
  else if (this->HaveCellScalars && !this->DrawingVertices && !this->PointPicking)
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

    if (actor->GetBackfaceProperty() && !this->DrawingVertices)
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

  if (this->HaveCellScalars && !this->DrawingVertices)
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
  bool hasAnisotropy = false;
  bool hasClearCoat = false;

  if (actor->GetProperty()->GetInterpolation() == VTK_PBR && lastLightComplexity > 0)
  {
    // PBR functions
    vtkShaderProgram::Substitute(FSSource, "//VTK::Light::Dec", vtkPBRFunctions);

    // disable default behavior with textures
    vtkShaderProgram::Substitute(FSSource, "//VTK::TCoord::Impl", "");

    // get color and material from textures
    std::vector<texinfo> textures = this->GetTextures(actor);
    bool albedo = false;
    bool material = false;
    bool emissive = false;
    toString.clear();

    if (this->HaveTCoords(this->CurrentInput) && !this->DrawingVertices)
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
        // Anisotropy texture is sampled in ReplaceShaderNormal
      }
    }

    vtkOpenGLRenderer* oglRen = vtkOpenGLRenderer::SafeDownCast(ren);

    // IBL
    if (oglRen && ren->GetUseImageBasedLighting() && ren->GetEnvironmentTexture())
    {
      hasIBL = true;
      toString << "  const float prefilterMaxLevel = float("
               << (oglRen->GetEnvMapPrefiltered()->GetPrefilterLevels() - 1) << ");\n";
    }

    if (!albedo)
    {
      // VTK colors are expressed in linear color space
      toString << "vec3 albedo = diffuseColor;\n";
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

    if (actor->GetProperty()->GetAnisotropy() != 0.0 &&
      this->VBOs->GetNumberOfComponents("normalMC") == 3 &&
      this->VBOs->GetNumberOfComponents("tangentMC") == 3)
    {
      // anisotropy, tangentVC and bitangentVC are defined
      hasAnisotropy = true;

      // Load anisotropic functions
      vtkShaderProgram::Substitute(FSSource, "//VTK::Define::Dec",
        "#define ANISOTROPY\n"
        "//VTK::Define::Dec");

      // Precompute anisotropic parameters
      // at and ab are the roughness along the tangent and bitangent
      // Disney, as in OSPray
      toString << "  float r2 = roughness * roughness;\n"
                  "  float aspect = sqrt(1.0 - 0.9 * anisotropy);\n";
      toString << "  float at = max(r2 / aspect, 0.001);\n"
                  "  float ab = max(r2 * aspect, 0.001);\n";

      toString << "  float TdV = dot(tangentVC, V);\n"
                  "  float BdV = dot(bitangentVC, V);\n";
    }

    hasClearCoat = actor->GetProperty()->GetCoatStrength() > 0.0;
    if (hasClearCoat)
    {
      // Load clear coat uniforms
      vtkShaderProgram::Substitute(FSSource, "//VTK::Define::Dec",
        "#define CLEAR_COAT\n"
        "//VTK::Define::Dec");

      // Clear coat parameters
      toString << "  vec3 coatN = coatNormalVCVSOutput;\n";
      toString << "  float coatRoughness = coatRoughnessUniform;\n";
      toString << "  float coatStrength = coatStrengthUniform;\n";
      toString << "  float coatNdV = clamp(dot(coatN, V), 1e-5, 1.0);\n";
    }

    if (hasIBL)
    {
      if (!oglRen->GetUseSphericalHarmonics())
      {
        toString << "  vec3 irradiance = texture(irradianceTex, envMatrix*N).rgb;\n";
      }
      else
      {
        toString << "  vec3 rotN = envMatrix * N;\n";
        toString << "  vec3 irradiance = vec3(ComputeSH(rotN, shRed), ComputeSH(rotN, shGreen), "
                    "ComputeSH(rotN, shBlue));\n";
      }

      if (hasAnisotropy)
      {
        toString << "  vec3 anisotropicTangent = cross(bitangentVC, V);\n"
                    "  vec3 anisotropicNormal = cross(anisotropicTangent, bitangentVC);\n"
                    "  vec3 bentNormal = normalize(mix(N, anisotropicNormal, anisotropy));\n"
                    "  vec3 worldReflect = normalize(envMatrix*reflect(-V, bentNormal));\n";
      }
      else
      {
        toString << "  vec3 worldReflect = normalize(envMatrix*reflect(-V, N));\n";
      }

      toString << "  vec3 prefilteredSpecularColor = textureLod(prefilterTex, worldReflect,"
                  " roughness * prefilterMaxLevel).rgb;\n";
      toString << "  vec2 brdf = texture(brdfTex, vec2(NdV, roughness)).rg;\n";

      // Use the same prefilter texture for clear coat but with the clear coat roughness and normal

      if (hasClearCoat)
      {
        toString
          << "  vec3 coatWorldReflect = normalize(envMatrix*reflect(-V,coatN));\n"
             "  vec3 prefilteredSpecularCoatColor = textureLod(prefilterTex, coatWorldReflect,"
             " coatRoughness * prefilterMaxLevel).rgb;\n"
             "  vec2 coatBrdf = texture(brdfTex, vec2(coatNdV, coatRoughness)).rg;\n";
      }
    }
    else
    {
      toString << "  vec3 irradiance = vec3(0.0);\n";
      toString << "  vec3 prefilteredSpecularColor = vec3(0.0);\n";
      toString << "  vec2 brdf = vec2(0.0, 0.0);\n";

      if (hasClearCoat)
      {
        toString << "  vec3 prefilteredSpecularCoatColor = vec3(0.0);\n";
        toString << "  vec2 coatBrdf = vec2(0.0);\n";
      }
    }

    toString << "  vec3 Lo = vec3(0.0);\n";

    if (lastLightComplexity != 0)
    {
      toString << "  vec3 F0 = mix(vec3(baseF0Uniform), albedo, metallic);\n"
                  // specular occlusion, it affects only material with an f0 < 0.02,
                  // else f90 is 1.0
                  "  float f90 = clamp(dot(F0, vec3(50.0 * 0.33)), 0.0, 1.0);\n"
                  "  vec3 F90 = mix(vec3(f90), edgeTintUniform, metallic);\n"
                  "  vec3 L, H, radiance, F, specular, diffuse;\n"
                  "  float NdL, NdH, HdL, distanceVC, attenuation, D, Vis;\n\n";
      if (hasClearCoat)
      {
        // Coat layer is dielectric so F0 and F90 are achromatic
        toString << "  vec3 coatF0 = vec3(coatF0Uniform);\n"
                    "  vec3 coatF90 = vec3(1.0);\n"
                    "  vec3 coatLayer, Fc;\n"
                    "  float coatNdL, coatNdH;\n"
                    "  vec3 coatColorFactor = mix(vec3(1.0), coatColorUniform, coatStrength);\n";
      }
    }

    toString << "//VTK::Light::Impl\n";

    vtkShaderProgram::Substitute(FSSource, "//VTK::Light::Impl", toString.str(), false);
    toString.clear();
    toString.str("");

    if (hasIBL)
    {
      toString << "//VTK::Light::Dec\n"
                  "uniform mat3 envMatrix;"
                  "uniform sampler2D brdfTex;\n"
                  "uniform samplerCube prefilterTex;\n";

      if (oglRen->GetUseSphericalHarmonics())
      {
        toString << "uniform float shRed[9];\n"
                    "uniform float shGreen[9];\n"
                    "uniform float shBlue[9];\n"
                    "float ComputeSH(vec3 n, float sh[9])\n"
                    "{\n"
                    "  float v = 0.0;\n"
                    "  v += sh[0];\n"
                    "  v += sh[1] * n.y;\n"
                    "  v += sh[2] * n.z;\n"
                    "  v += sh[3] * n.x;\n"
                    "  v += sh[4] * n.x * n.y;\n"
                    "  v += sh[5] * n.y * n.z;\n"
                    "  v += sh[6] * (3.0 * n.z * n.z - 1.0);\n"
                    "  v += sh[7] * n.x * n.z;\n"
                    "  v += sh[8] * (n.x * n.x - n.y * n.y);\n"
                    "  return max(v, 0.0);\n"
                    "}\n";
      }
      else
      {
        toString << "uniform samplerCube irradianceTex;\n";
      }

      // add uniforms
      vtkShaderProgram::Substitute(FSSource, "//VTK::Light::Dec", toString.str());
      toString.clear();
      toString.str("");
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
        if (hasAnisotropy)
        {
          // When V=H, maybe can be optimised
          toString << "specular = SpecularAnisotropic(at, ab, V, tangentVC, bitangentVC, V, TdV, "
                      "BdV, NdV, NdV, NdV,\n"
                      "1.0, roughness, anisotropy, F0, F90, F);\n";
        }
        else
        {
          toString << "specular = SpecularIsotropic(NdV, NdV, NdV, 1.0, roughness, F0, F90, F);\n";
        }
        toString << "  diffuse = (1.0 - metallic) * (1.0 - F) * DiffuseLambert(albedo);\n"
                    "  radiance = lightColor0;\n";

        if (hasClearCoat)
        {
          toString << "  // Clear coat is isotropic\n"
                      "  coatLayer = SpecularIsotropic(coatNdV, coatNdV, coatNdV, 1.0,"
                      " coatRoughness, coatF0, coatF90, Fc) * radiance * coatNdV * coatStrength;\n"
                      "  Fc *= coatStrength;\n"
                      "  radiance *= coatColorFactor;\n"
                      "  specular *= (1.0 - Fc) * (1.0 - Fc);\n"
                      "  diffuse *= (1.0 - Fc);\n"
                      "  Lo += coatLayer;\n";
        }
        toString << "  Lo += radiance * (diffuse + specular) * NdV;\n\n"
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
                      "  HdL = clamp(dot(H, L), 1e-5, 1.0);\n"
                      "  NdL = clamp(dot(N, L), 1e-5, 1.0);\n"
                      "  NdH = clamp(dot(N, H), 1e-5, 1.0);\n"
                      "  radiance = lightColor"
                   << i << ";\n";

          if (hasAnisotropy)
          {
            toString << "  specular = SpecularAnisotropic(at, ab, L, tangentVC, bitangentVC, H, "
                        "TdV, BdV, NdH, NdV, NdL, HdL, roughness, anisotropy, F0, F90, F);\n";
          }
          else
          {
            toString
              << "  specular = SpecularIsotropic(NdH, NdV, NdL, HdL, roughness, F0, F90, F);\n";
          }

          toString << "  diffuse = (1.0 - metallic) * (1.0 - F) * DiffuseLambert(albedo);\n";

          if (hasClearCoat)
          {
            toString
              << "  coatNdL = clamp(dot(coatN, L), 1e-5, 1.0);\n"
                 "  coatNdH = clamp(dot(coatN, H), 1e-5, 1.0);\n"
                 "  // Clear coat is isotropic\n"
                 "  coatLayer = SpecularIsotropic(coatNdH, coatNdV, coatNdL, HdL,"
                 " coatRoughness, coatF0, coatF90, Fc) * radiance * coatNdL * coatStrength;\n"
                 "  // Energy compensation depending on how much light is reflected by the "
                 "coat layer\n"
                 "  Fc *= coatStrength;\n"
                 "  specular *= (1.0 - Fc) * (1.0 - Fc);\n"
                 "  diffuse *= (1.0 - Fc);\n"
                 "  radiance *= coatColorFactor;\n"
                 "  Lo += coatLayer;\n";
          }

          toString << "  Lo += radiance * (diffuse + specular) * NdL;\n";
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
                      "  HdL = clamp(dot(H, L), 1e-5, 1.0);\n"
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
                   << i << " * attenuation;\n";

          if (hasAnisotropy)
          {
            toString << "  specular = SpecularAnisotropic(at, ab, L, tangentVC, bitangentVC, H, "
                        "TdV, BdV, NdH, NdV, NdL, HdL, roughness, anisotropy, F0, F90, F);\n";
          }
          else
          {
            toString
              << "  specular = SpecularIsotropic(NdH, NdV, NdL, HdL, roughness, F0, F90, F);\n";
          }

          toString << "  diffuse = (1.0 - metallic) * (1.0 - F) * DiffuseLambert(albedo);\n";

          if (hasClearCoat)
          {
            toString
              << "  coatNdL = clamp(dot(coatN, L), 1e-5, 1.0);\n"
                 "  coatNdH = clamp(dot(coatN, H), 1e-5, 1.0);\n"
                 "  // Clear coat is isotropic\n"
                 "  coatLayer = SpecularIsotropic(coatNdH, coatNdV, coatNdL, HdL,"
                 " coatRoughness, coatF0, coatF90, Fc) * radiance * coatNdL * coatStrength;\n"
                 "  // Energy compensation depending on how much light is reflected by the "
                 "coat layer\n"
                 "  Fc *= coatStrength;\n"
                 "  specular *= (1.0 - Fc) * (1.0 - Fc);\n"
                 "  diffuse *= (1.0 - Fc);\n"
                 "  radiance *= coatColorFactor;\n"
                 "  Lo += coatLayer;\n";
          }

          toString << "  Lo += radiance * (diffuse + specular) * NdL;\n";
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
    toString.clear();
    toString.str("");

    toString << "  // In IBL, we assume that v=n, so the amount of light reflected is\n"
                "  // the reflectance F0\n"
                "  vec3 specularBrdf = F0 * brdf.r + F90 * brdf.g;\n"
                "  vec3 iblSpecular = prefilteredSpecularColor * specularBrdf;\n"
                // no diffuse for metals
                "  vec3 iblDiffuse = (1.0 - F0) * (1.0 - metallic) * irradiance * albedo;\n"
                "  vec3 color = iblDiffuse + iblSpecular;\n"
                "\n";

    if (hasClearCoat)
    {
      toString
        << "  // Clear coat attenuation\n"
           "  Fc = F_Schlick(coatF0, coatF90, coatNdV) * coatStrength;\n"
           "  iblSpecular *= (1.0 - Fc);\n"
           "  iblDiffuse *= (1.0 - Fc) * (1.0 - Fc);\n"
           "  // Clear coat specular\n"
           "  vec3 iblSpecularClearCoat = prefilteredSpecularCoatColor * (coatF0 * coatBrdf.r + "
           "coatBrdf.g) * Fc;\n"
           // Color absorption by the coat layer
           "  color *= coatColorFactor;\n"
           "  color += iblSpecularClearCoat;\n"
           "\n";
    }

    toString << "  color += Lo;\n"
                "  color = mix(color, color * ao, aoStrengthUniform);\n" // ambient occlusion
                "  color += emissiveColor;\n"                            // emissive
                "  color = pow(color, vec3(1.0/2.2));\n"                 // to sRGB color space
                "  gl_FragData[0] = vec4(color, opacity);\n"
                "  //VTK::Light::Impl";

    vtkShaderProgram::Substitute(FSSource, "//VTK::Light::Impl", toString.str(), false);
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
  if (this->DrawingVertices)
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
  for (const auto& it : textures)
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
      textures[i].second == "emissiveTex" || textures[i].second == "anisotropyTex" ||
      textures[i].second == "coatNormalTex")
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
          "  int idx = vertexIDVSOutput;\n"
          "  gl_FragData[0] = vec4(float(idx%256)/255.0, float((idx/256)%256)/255.0, "
          "float((idx/65536)%256)/255.0, 1.0);\n");
        break;

      case vtkHardwareSelector::POINT_ID_HIGH24:
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
          "  int idx = vertexIDVSOutput;\n idx = ((idx & 0xff000000) >> 24);\n"
          "  gl_FragData[0] = vec4(float(idx)/255.0, 0.0, 0.0, 1.0);\n");
        break;

      // cell ID is just gl_PrimitiveID
      case vtkHardwareSelector::CELL_ID_LOW24:
        vtkShaderProgram::Substitute(FSSource, "//VTK::Picking::Impl",
          "  int idx = gl_PrimitiveID + PrimitiveIDOffset;\n"
          "  gl_FragData[0] = vec4(float(idx%256)/255.0, float((idx/256)%256)/255.0, "
          "float((idx/65536)%256)/255.0, 1.0);\n");
        break;

      case vtkHardwareSelector::CELL_ID_HIGH24:
        // if (selector &&
        //     selector->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_POINTS)
        vtkShaderProgram::Substitute(FSSource, "//VTK::Picking::Impl",
          "  int idx = (gl_PrimitiveID + PrimitiveIDOffset);\n idx = ((idx & 0xff000000) >> "
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

    // when point picking always move fragments to the closest point
    // to the camera.
    if (this->PointPicking)
    {
      vtkShaderProgram::Substitute(FSSource, "//VTK::Depth::Impl",
        "  vec3 normalVCVSOutput = vec3(0.0,0.0,1.0);\n"
        "  gl_FragDepth = gl_FragCoord.z + ZCalcS*ZCalcR;\n"
        "  if (cameraParallel == 0)\n"
        "  {\n"
        "    float ZCalcQ = (ZCalcR - 1.0);\n"
        "    gl_FragDepth = (ZCalcS - gl_FragCoord.z) / ZCalcQ + ZCalcS;\n"
        "  }\n"
        "//VTK::Depth::Impl");
    }
    else
    {
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
        "  }\n"
        "//VTK::Depth::Impl");
    }

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
      "  }\n"
      "//VTK::Depth::Impl");
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
    std::ostringstream toString;

    bool hasClearCoat = actor->GetProperty()->GetInterpolation() == VTK_PBR &&
      actor->GetProperty()->GetCoatStrength() > 0.0;

    if (this->VBOs->GetNumberOfComponents("normalMC") == 3)
    {
      vtkShaderProgram::Substitute(VSSource, "//VTK::Normal::Dec",
        "//VTK::Normal::Dec\n"
        "in vec3 normalMC;\n"
        "uniform mat3 normalMatrix;\n"
        "out vec3 normalVCVSOutput;");
      vtkShaderProgram::Substitute(VSSource, "//VTK::Normal::Impl",
        "normalVCVSOutput = normalMatrix * normalMC;\n"
        "//VTK::Normal::Impl");
      vtkShaderProgram::Substitute(GSSource, "//VTK::Normal::Dec",
        "//VTK::Normal::Dec\n"
        "in vec3 normalVCVSOutput[];\n"
        "out vec3 normalVCGSOutput;");
      vtkShaderProgram::Substitute(GSSource, "//VTK::Normal::Impl",
        "//VTK::Normal::Impl\n"
        "normalVCGSOutput = normalVCVSOutput[i];");
      vtkShaderProgram::Substitute(FSSource, "//VTK::Normal::Dec",
        "//VTK::Normal::Dec\n"
        "uniform mat3 normalMatrix;\n"
        "in vec3 normalVCVSOutput;");

      toString << "vec3 normalVCVSOutput = normalize(normalVCVSOutput);\n"
                  //  if (!gl_FrontFacing) does not work in intel hd4000 mac
                  //  if (int(gl_FrontFacing) == 0) does not work on mesa
                  "  if (gl_FrontFacing == false) { normalVCVSOutput = -normalVCVSOutput; }\n";
      //"normalVC = normalVCVarying;";
      if (hasClearCoat)
      {
        toString << "vec3 coatNormalVCVSOutput = normalVCVSOutput;\n";
      }
      toString << "//VTK::Normal::Impl";
      vtkShaderProgram::Substitute(FSSource, "//VTK::Normal::Impl", toString.str());

      // normal mapping
      std::vector<texinfo> textures = this->GetTextures(actor);
      bool normalMapping = std::find_if(textures.begin(), textures.end(), [](const texinfo& tex) {
        return tex.second == "normalTex";
      }) != textures.end();
      bool coatNormalMapping = hasClearCoat &&
        std::find_if(textures.begin(), textures.end(),
          [](const texinfo& tex) { return tex.second == "coatNormalTex"; }) != textures.end();

      bool hasAnisotropy = actor->GetProperty()->GetInterpolation() == VTK_PBR &&
        actor->GetProperty()->GetAnisotropy() != 0.0;

      // if we have points tangents, we need it for normal mapping, coat normal mapping and
      // anisotropy
      if (this->VBOs->GetNumberOfComponents("tangentMC") == 3 && !this->DrawingVertices &&
        (normalMapping || coatNormalMapping || hasAnisotropy))
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
          "in vec3 tangentVCVSOutput;\n");
        vtkShaderProgram::Substitute(FSSource, "//VTK::Normal::Impl",
          " vec3 tangentVC = tangentVCVSOutput;\n"
          "//VTK::Normal::Impl");

        if (hasAnisotropy)
        {
          // We need to rotate the anisotropy direction (the tangent) by anisotropyRotation * 2 *
          // PI
          vtkShaderProgram::Substitute(FSSource, "//VTK::Normal::Dec",
            "//VTK::Normal::Dec\n"
            "uniform float anisotropyRotationUniform;\n");

          bool rotationMap = std::find_if(textures.begin(), textures.end(), [](const texinfo& tex) {
            return tex.second == "anisotropyTex";
          }) != textures.end();
          if (rotationMap)
          {
            // Sample the texture
            vtkShaderProgram::Substitute(FSSource, "//VTK::Normal::Impl",
              "  vec2 anisotropySample = texture(anisotropyTex, tcoordVCVSOutput).rg;\n"
              "  float anisotropy = anisotropySample.x * anisotropyUniform;\n"
              "  float anisotropyRotation = anisotropySample.y * anisotropyRotationUniform;\n"
              "//VTK::Normal::Impl");
          }
          else
          {
            vtkShaderProgram::Substitute(FSSource, "//VTK::Normal::Impl",
              "  float anisotropy = anisotropyUniform;\n"
              "  float anisotropyRotation = anisotropyRotationUniform;\n"
              "//VTK::Normal::Impl");
          }
          vtkShaderProgram::Substitute(FSSource, "//VTK::Normal::Impl",
            "  // Rotate the anisotropy direction (tangent) around the normal with a rotation "
            "factor\n"
            "  float r2pi = anisotropyRotation * 2 * PI;\n"
            "  float s = - sin(r2pi);\n" // Counter clockwise (as in
                                         // OSPray)
            "  float c = cos(r2pi);\n"
            "  vec3 Nn = normalize(normalVCVSOutput);\n"
            "  tangentVC = (1.0-c) * dot(tangentVCVSOutput,Nn) * Nn\n"
            "+ c * tangentVCVSOutput - s * cross(Nn, tangentVCVSOutput);\n"
            "//VTK::Normal::Impl");
        }

        vtkShaderProgram::Substitute(FSSource, "//VTK::Normal::Impl",
          "  tangentVC = normalize(tangentVC - dot(tangentVC, "
          "normalVCVSOutput) * normalVCVSOutput);\n"
          "  vec3 bitangentVC = cross(normalVCVSOutput, tangentVC);\n"
          "//VTK::Normal::Impl");

        if (normalMapping || coatNormalMapping)
        {
          vtkShaderProgram::Substitute(FSSource, "//VTK::Normal::Impl",
            "  mat3 tbn = mat3(tangentVC, bitangentVC, normalVCVSOutput);\n"
            "//VTK::Normal::Impl");

          if (normalMapping)
          {
            vtkShaderProgram::Substitute(FSSource, "//VTK::Normal::Dec",
              "//VTK::Normal::Dec\n"
              "uniform float normalScaleUniform;\n");

            vtkShaderProgram::Substitute(FSSource, "//VTK::Normal::Impl",
              "  vec3 normalTS = texture(normalTex, tcoordVCVSOutput).xyz * 2.0 - 1.0;\n"
              "  normalTS = normalize(normalTS * vec3(normalScaleUniform, normalScaleUniform, "
              "1.0));\n"
              "  normalVCVSOutput = normalize(tbn * normalTS);\n"
              "//VTK::Normal::Impl");
          }
          if (coatNormalMapping)
          {
            vtkShaderProgram::Substitute(FSSource, "//VTK::Normal::Dec",
              "//VTK::Normal::Dec\n"
              "uniform float coatNormalScaleUniform;\n");

            vtkShaderProgram::Substitute(FSSource, "//VTK::Normal::Impl",
              "  vec3 coatNormalTS = texture(coatNormalTex, tcoordVCVSOutput).xyz * 2.0 - 1.0;\n"
              "  coatNormalTS = normalize(coatNormalTS * vec3(coatNormalScaleUniform, "
              "coatNormalScaleUniform, "
              "1.0));\n"
              "  coatNormalVCVSOutput = normalize(tbn * coatNormalTS);\n"
              "//VTK::Normal::Impl");
          }
        }
      }

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

      toString.clear();
      toString.str("");
      if (this->CellNormalTexture->GetVTKDataType() == VTK_FLOAT)
      {
        toString << "vec3 normalVCVSOutput = \n"
                    "    texelFetchBuffer(textureN, gl_PrimitiveID + PrimitiveIDOffset).xyz;\n"
                    "normalVCVSOutput = normalize(normalMatrix * normalVCVSOutput);\n"
                    "  if (gl_FrontFacing == false) { normalVCVSOutput = -normalVCVSOutput; }\n";
      }
      else
      {
        toString << "vec3 normalVCVSOutput = \n"
                    "    texelFetchBuffer(textureN, gl_PrimitiveID + PrimitiveIDOffset).xyz;\n"
                    "normalVCVSOutput = normalVCVSOutput * 255.0/127.0 - 1.0;\n"
                    "normalVCVSOutput = normalize(normalMatrix * normalVCVSOutput);\n"
                    "  if (gl_FrontFacing == false) { normalVCVSOutput = -normalVCVSOutput; }\n";
      }

      if (hasClearCoat)
      {
        toString << "vec3 coatNormalVCVSOutput = normalVCVSOutput;\n";
      }

      vtkShaderProgram::Substitute(FSSource, "//VTK::Normal::Impl", toString.str());
      shaders[vtkShader::Fragment]->SetSource(FSSource);
      return;
    }

    toString.clear();
    toString.str("");
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

      toString << "vec3 normalVCVSOutput;\n"
                  "  fdx = normalize(fdx);\n"
                  "  fdy = normalize(fdy);\n"
                  "  if (abs(fdx.x) > 0.0)\n"
                  "    { normalVCVSOutput = normalize(cross(vec3(fdx.y, -fdx.x, 0.0), fdx)); }\n"
                  "  else { normalVCVSOutput = normalize(cross(vec3(fdy.y, -fdy.x, 0.0), fdy));}\n";
    }
    else // not lines, so surface
    {
      vtkShaderProgram::Substitute(FSSource, "//VTK::UniformFlow::Impl",
        "vec3 fdx = dFdx(vertexVC.xyz);\n"
        "  vec3 fdy = dFdy(vertexVC.xyz);\n"
        "  //VTK::UniformFlow::Impl\n" // For further replacements
      );

      toString << "  vec3 normalVCVSOutput = normalize(cross(fdx,fdy));\n"
                  "  if (cameraParallel == 1 && normalVCVSOutput.z < 0.0) { normalVCVSOutput = "
                  "-1.0*normalVCVSOutput; }\n"
                  "  if (cameraParallel == 0 && dot(normalVCVSOutput,vertexVC.xyz) > 0.0) { "
                  "normalVCVSOutput "
                  "= -1.0*normalVCVSOutput; }\n";
    }

    if (hasClearCoat)
    {
      toString << "vec3 coatNormalVCVSOutput = normalVCVSOutput;\n";
    }
    vtkShaderProgram::Substitute(FSSource, "//VTK::Normal::Impl", toString.str());
    shaders[vtkShader::Fragment]->SetSource(FSSource);
  }
}

void vtkOpenGLPolyDataMapper::ReplaceShaderPositionVC(
  std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer*, vtkActor* actor)
{
  std::string VSSource = shaders[vtkShader::Vertex]->GetSource();
  std::string GSSource = shaders[vtkShader::Geometry]->GetSource();
  std::string FSSource = shaders[vtkShader::Fragment]->GetSource();

  vtkShaderProgram::Substitute(
    FSSource, "//VTK::Camera::Dec", "uniform int cameraParallel;\n", false);

  // do we need the vertex in the shader in View Coordinates
  if (this->LastLightComplexity[this->LastBoundBO] > 0 ||
    this->DrawingTubes(*this->LastBoundBO, actor))
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

  // if we need an offset handle it here
  // The value of .000016 is suitable for depth buffers
  // of at least 16 bit depth. We do not query the depth
  // right now because we would need some mechanism to
  // cache the result taking into account FBO changes etc.
  if (factor != 0.0 || offset != 0.0)
  {
    std::string FSSource = shaders[vtkShader::Fragment]->GetSource();

    if (ren->GetActiveCamera()->GetParallelProjection())
    {
      vtkShaderProgram::Substitute(FSSource, "//VTK::Coincident::Dec", "uniform float cCValue;");
      if (this->DrawingTubesOrSpheres(*this->LastBoundBO, actor))
      {
        vtkShaderProgram::Substitute(
          FSSource, "//VTK::Depth::Impl", "gl_FragDepth = gl_FragDepth + cCValue;\n");
      }
      else
      {
        vtkShaderProgram::Substitute(
          FSSource, "//VTK::Depth::Impl", "gl_FragDepth = gl_FragCoord.z + cCValue;\n");
      }
    }
    else
    {
      vtkShaderProgram::Substitute(FSSource, "//VTK::Coincident::Dec",
        "uniform float cCValue;\n"
        "uniform float cSValue;\n"
        "uniform float cDValue;");
      if (this->DrawingTubesOrSpheres(*this->LastBoundBO, actor))
      {
        vtkShaderProgram::Substitute(FSSource, "//VTK::Depth::Impl",
          "float Zdc = gl_FragDepth*2.0 - 1.0;\n"
          "  float Z2 = -1.0*cDValue/(Zdc + cCValue) + cSValue;\n"
          "  float Zdc2 = -1.0*cCValue - cDValue/Z2;\n"
          "  gl_FragDepth = Zdc2*0.5 + 0.5;\n");
      }
      else
      {
        vtkShaderProgram::Substitute(FSSource, "//VTK::Depth::Impl",
          "float Zdc = gl_FragCoord.z*2.0 - 1.0;\n"
          "  float Z2 = -1.0*cDValue/(Zdc + cCValue) + cSValue;\n"
          "  float Zdc2 = -1.0*cCValue - cDValue/Z2;\n"
          "  gl_FragDepth = Zdc2*0.5 + 0.5;\n");
      }
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
  this->ReplaceShaderEdges(shaders, ren, actor);
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

//------------------------------------------------------------------------------
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
    bool isTrisOrStrips = (cellBO.PrimitiveType == PrimitiveTris ||
      cellBO.PrimitiveType == vtkOpenGLPolyDataMapper::PrimitiveTriStrips);
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
    (vtkOpenGLRenderer::SafeDownCast(ren)->GetUseSphericalHarmonics() ? 0x40 : 0) +
    (actor->GetProperty()->GetCoatStrength() > 0.0 ? 0x80 : 0) +
    (actor->GetProperty()->GetAnisotropy() > 0.0 ? 0x100 : 0) +
    ((this->VBOs->GetNumberOfComponents("tcoord") % 4) << 9);

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

//------------------------------------------------------------------------------
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
  vtkOpenGLCheckErrorMacro("failed after UpdateShader");

  if (cellBO.Program)
  {
    this->SetCustomUniforms(cellBO, actor);
    vtkOpenGLCheckErrorMacro("failed after UpdateShader");
    this->SetMapperShaderParameters(cellBO, ren, actor);
    vtkOpenGLCheckErrorMacro("failed after UpdateShader");
    this->SetPropertyShaderParameters(cellBO, ren, actor);
    vtkOpenGLCheckErrorMacro("failed after UpdateShader");
    this->SetCameraShaderParameters(cellBO, ren, actor);
    vtkOpenGLCheckErrorMacro("failed after UpdateShader");
    this->SetLightingShaderParameters(cellBO, ren, actor);
    vtkOpenGLCheckErrorMacro("failed after UpdateShader");

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

  vtkOpenGLCheckErrorMacro("failed after UpdateShader");

  // Add IBL textures
  if (ren->GetUseImageBasedLighting() && ren->GetEnvironmentTexture())
  {
    vtkOpenGLRenderer* oglRen = vtkOpenGLRenderer::SafeDownCast(ren);
    if (oglRen)
    {
      cellBO.Program->SetUniformi("brdfTex", oglRen->GetEnvMapLookupTable()->GetTextureUnit());
      cellBO.Program->SetUniformi("prefilterTex", oglRen->GetEnvMapPrefiltered()->GetTextureUnit());

      if (!oglRen->GetUseSphericalHarmonics())
      {
        cellBO.Program->SetUniformi(
          "irradianceTex", oglRen->GetEnvMapIrradiance()->GetTextureUnit());
      }
    }
  }
  vtkOpenGLCheckErrorMacro("failed after UpdateShader");

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

  vtkOpenGLCheckErrorMacro("failed after UpdateShader");

  if (cellBO.Program->IsUniformUsed("edgeTexture"))
  {
    int tunit = this->EdgeTexture->GetTextureUnit();
    cellBO.Program->SetUniformi("edgeTexture", tunit);
  }
  vtkOpenGLCheckErrorMacro("failed after UpdateShader");
  if (this->DrawingEdges(ren, actor))
  {
    float lw = actor->GetProperty()->GetLineWidth();
    cellBO.Program->SetUniformf("lineWidth", lw < 1.1 ? 1.1 : lw);
    int vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    float dims[4];
    dims[0] = vp[0];
    dims[1] = vp[1];
    dims[2] = vp[2];
    dims[3] = vp[3];
    cellBO.Program->SetUniform4f("vpDims", dims);
    cellBO.Program->SetUniform3f("edgeColor", actor->GetProperty()->GetEdgeColor());
  }
  vtkOpenGLCheckErrorMacro("failed after UpdateShader");

  if ((this->HaveCellScalars) && cellBO.Program->IsUniformUsed("textureC"))
  {
    int tunit = this->CellScalarTexture->GetTextureUnit();
    cellBO.Program->SetUniformi("textureC", tunit);
  }
  vtkOpenGLCheckErrorMacro("failed after UpdateShader");

  if (this->HaveCellNormals && cellBO.Program->IsUniformUsed("textureN"))
  {
    int tunit = this->CellNormalTexture->GetTextureUnit();
    cellBO.Program->SetUniformi("textureN", tunit);
  }
  vtkOpenGLCheckErrorMacro("failed after UpdateShader");

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
  vtkOpenGLCheckErrorMacro("failed after UpdateShader");

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
  vtkOpenGLCheckErrorMacro("failed after UpdateShader");

  // handle wide lines
  if (this->HaveWideLines(ren, actor) && cellBO.Program->IsUniformUsed("lineWidthNVC"))
  {
    int vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);

    float propLineWidth = actor->GetProperty()->GetLineWidth();

    if (this->DrawingSelection)
    {
      propLineWidth = actor->GetProperty()->GetSelectionLineWidth();
    }

    float lineWidth[2];
    lineWidth[0] = 2.0 * propLineWidth / vp[2];
    lineWidth[1] = 2.0 * propLineWidth / vp[3];
    cellBO.Program->SetUniform2f("lineWidthNVC", lineWidth);
  }
  vtkOpenGLCheckErrorMacro("failed after UpdateShader");
}

//------------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::SetLightingShaderParameters(
  vtkOpenGLHelper& cellBO, vtkRenderer* ren, vtkActor*)
{
  // for unlit there are no lighting parameters
  if (this->LastLightComplexity[&cellBO] < 1)
  {
    return;
  }

  vtkOpenGLRenderer* oglRen = vtkOpenGLRenderer::SafeDownCast(ren);
  if (oglRen)
  {
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

        cellBO.Program->SetUniform1fv(uniforms[i].c_str(), 9, coeffs);
      }
    }
  }

  vtkShaderProgram* program = cellBO.Program;
  vtkOpenGLRenderer* oren = static_cast<vtkOpenGLRenderer*>(ren);
  oren->UpdateLightingUniforms(program);
}

//------------------------------------------------------------------------------
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

  vtkNew<vtkMatrix3x3> env;
  if (program->IsUniformUsed("envMatrix"))
  {
    double up[3];
    double right[3];
    double front[3];
    ren->GetEnvironmentUp(up);
    ren->GetEnvironmentRight(right);
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
  vtkOpenGLVertexBufferObject* vvbo = this->VBOs->GetVBO("vertexMC");
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

//------------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::SetPropertyShaderParameters(
  vtkOpenGLHelper& cellBO, vtkRenderer*, vtkActor* actor)
{
  vtkShaderProgram* program = cellBO.Program;

  vtkProperty* ppty = actor->GetProperty();

  {
    // Query the property for some of the properties that can be applied.
    float opacity = this->DrawingSelection ? 1.f : static_cast<float>(ppty->GetOpacity());
    double* aColor = this->DrawingVertices ? ppty->GetVertexColor() : ppty->GetAmbientColor();
    double aIntensity = ((this->DrawingVertices || this->DrawingSelection) &&
                          !this->DrawingTubesOrSpheres(cellBO, actor))
      ? 1.0
      : ppty->GetAmbient();

    if (this->DrawingSelection)
    {
      aColor = ppty->GetSelectionColor();
      opacity = static_cast<float>(aColor[3]);
    }

    double* dColor = this->DrawingVertices ? ppty->GetVertexColor() : ppty->GetDiffuseColor();
    double dIntensity = ((this->DrawingVertices || this->DrawingSelection) &&
                          !this->DrawingTubesOrSpheres(cellBO, actor))
      ? 0.0
      : ppty->GetDiffuse();

    double* sColor = ppty->GetSpecularColor();
    double sIntensity =
      (this->DrawingVertices && !this->DrawingTubes(cellBO, actor)) ? 0.0 : ppty->GetSpecular();
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
      program->SetUniform3f("edgeTintUniform", ppty->GetEdgeTint());

      if (ppty->GetAnisotropy() > 0.0)
      {
        program->SetUniformf("anisotropyUniform", static_cast<float>(ppty->GetAnisotropy()));
        program->SetUniformf(
          "anisotropyRotationUniform", static_cast<float>(ppty->GetAnisotropyRotation()));
      }

      if (ppty->GetCoatStrength() > 0.0)
      {
        // Compute the reflectance of the coat layer and the exterior
        // Hard coded air environment (ior = 1.0)
        const double environmentIOR = 1.0;
        program->SetUniformf("coatF0Uniform",
          static_cast<float>(
            vtkProperty::ComputeReflectanceFromIOR(ppty->GetCoatIOR(), environmentIOR)));
        program->SetUniform3f("coatColorUniform", ppty->GetCoatColor());
        program->SetUniformf("coatStrengthUniform", static_cast<float>(ppty->GetCoatStrength()));
        program->SetUniformf("coatRoughnessUniform", static_cast<float>(ppty->GetCoatRoughness()));
        program->SetUniformf(
          "coatNormalScaleUniform", static_cast<float>(ppty->GetCoatNormalScale()));
      }
      // Compute the reflectance of the base layer
      program->SetUniformf(
        "baseF0Uniform", static_cast<float>(ppty->ComputeReflectanceOfBaseLayer()));
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
    (primType == PrimitiveTris || primType == vtkOpenGLPolyDataMapper::PrimitiveTriStrips))
  {
    // do something rough is better than nothing
    double zRes = this->GetResolveCoincidentTopologyZShift(); // 0 is no shift 1 is big shift
    double f = zRes * 4.0;
    offset = f;
  }

  vtkProperty* prop = actor->GetProperty();
  if ((this->GetResolveCoincidentTopology() == VTK_RESOLVE_POLYGON_OFFSET) ||
    (prop->GetEdgeVisibility() && prop->GetRepresentation() == VTK_SURFACE) ||
    this->DrawingSelection)
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
    else if (primType == PrimitiveTris || primType == vtkOpenGLPolyDataMapper::PrimitiveTriStrips)
    {
      this->GetCoincidentTopologyPolygonOffsetParameters(f, u);
    }
    factor = f;
    offset = u;
  }

  // always move selections a bit closer to the camera
  // but not as close as point picking would move
  if (this->DrawingSelection)
  {
    offset -= 1.0;
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

  // the maximum number of cells in a draw call is the max of
  // 1) the sum of IBO size divided by the stride
  // 2) the max of any used call in a cellIdArray
  vtkIdType maxCellId = 0;
  int representation = actor->GetProperty()->GetRepresentation();
  for (int i = vtkOpenGLPolyDataMapper::PrimitiveStart;
       i < vtkOpenGLPolyDataMapper::PrimitiveTriStrips + 1; i++)
  {
    if (this->Primitives[i].IBO->IndexCount)
    {
      GLenum mode = this->GetOpenGLMode(representation, i);
      if (this->PointPicking)
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

//------------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::RenderPieceStart(vtkRenderer* ren, vtkActor* actor)
{
  // render points for point picking in a special way
  // all cell types should be rendered as points
  vtkHardwareSelector* selector = ren->GetSelector();
  this->PointPicking = false;
  if (selector && selector->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    this->PointPicking = true;
  }

  // Set the PointSize and LineWidget
  vtkOpenGLRenderWindow* renWin = static_cast<vtkOpenGLRenderWindow*>(ren->GetRenderWindow());
  vtkOpenGLState* ostate = renWin->GetState();
  ostate->vtkglPointSize(actor->GetProperty()->GetPointSize());

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
  if (this->EdgeValues.size())
  {
    this->EdgeTexture->Activate();
  }

  // If we are coloring by texture, then load the texture map.
  // Use Map as indicator, because texture hangs around.
  if (this->ColorTextureMap)
  {
    this->InternalColorTexture->Load(ren);
  }

  this->LastBoundBO = nullptr;
}

//------------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::RenderPieceDraw(vtkRenderer* ren, vtkActor* actor)
{
  int representation = actor->GetProperty()->GetRepresentation();

  vtkOpenGLRenderWindow* renWin = static_cast<vtkOpenGLRenderWindow*>(ren->GetRenderWindow());
  vtkOpenGLState* ostate = renWin->GetState();

#ifndef GL_ES_VERSION_3_0
  // when using IBL, we need seamless cubemaps to avoid artifacts
  if (ren->GetUseImageBasedLighting() && ren->GetEnvironmentTexture())
  {
    ostate->vtkglEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
  }
#endif

  vtkHardwareSelector* selector = ren->GetSelector();
  bool draw_surface_with_edges =
    (actor->GetProperty()->GetEdgeVisibility() && representation == VTK_SURFACE) && !selector;
  int numVerts = this->VBOs->GetNumberOfTuples("vertexMC");
  for (int i = vtkOpenGLPolyDataMapper::PrimitiveStart;
       i < (draw_surface_with_edges ? vtkOpenGLPolyDataMapper::PrimitiveEnd
                                    : vtkOpenGLPolyDataMapper::PrimitiveTriStrips + 1);
       i++)
  {
    this->DrawingVertices = (i > vtkOpenGLPolyDataMapper::PrimitiveTriStrips ? true : false);
    this->DrawingSelection = false;
    if (this->Primitives[i].IBO->IndexCount)
    {
      GLenum mode = this->GetOpenGLMode(representation, i);
      if (this->PointPicking)
      {
        ostate->vtkglPointSize(this->GetPointPickingPrimitiveSize(i));
        mode = GL_POINTS;
      }

      // Update/build/etc the shader.
      this->UpdateShaders(this->Primitives[i], ren, actor);

      if (mode == GL_LINES && !this->HaveWideLines(ren, actor))
      {
        ostate->vtkglLineWidth(actor->GetProperty()->GetLineWidth());
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

    // Selection
    this->DrawingSelection = true;
    if (this->SelectionPrimitives[i].IBO->IndexCount)
    {
      GLenum mode = this->GetOpenGLMode(this->SelectionType, i);

      if (mode == GL_POINTS)
      {
        ostate->vtkglPointSize(actor->GetProperty()->GetSelectionPointSize());
      }

      // Update/build/etc the shader.
      this->UpdateShaders(this->SelectionPrimitives[i], ren, actor);

      this->SelectionPrimitives[i].IBO->Bind();
      glDrawRangeElements(mode, 0, static_cast<GLuint>(numVerts - 1),
        static_cast<GLsizei>(this->SelectionPrimitives[i].IBO->IndexCount), GL_UNSIGNED_INT,
        nullptr);
      this->SelectionPrimitives[i].IBO->Release();
    }
  }
}

//------------------------------------------------------------------------------
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

  if (this->EdgeValues.size())
  {
    this->EdgeTexture->Deactivate();
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

//------------------------------------------------------------------------------
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

  this->UpdateCameraShiftScale(ren, actor);
  this->RenderPieceStart(ren, actor);
  this->RenderPieceDraw(ren, actor);
  this->RenderPieceFinish(ren, actor);
}

void vtkOpenGLPolyDataMapper::UpdateCameraShiftScale(vtkRenderer* ren, vtkActor* actor)
{
  // handle camera shift scale
  if (this->ShiftScaleMethod == vtkOpenGLVertexBufferObject::NEAR_PLANE_SHIFT_SCALE ||
    this->ShiftScaleMethod == vtkOpenGLVertexBufferObject::FOCAL_POINT_SHIFT_SCALE)
  {
    // get ideal shift scale from camera
    auto posVBO = this->VBOs->GetVBO("vertexMC");
    if (posVBO)
    {
      posVBO->SetCamera(ren->GetActiveCamera());
      posVBO->SetProp3D(actor);
      posVBO->UpdateShiftScale(this->CurrentInput->GetPoints()->GetData());
      // force a rebuild if needed
      if (posVBO->GetMTime() > posVBO->GetUploadTime())
      {
        posVBO->UploadDataArray(this->CurrentInput->GetPoints()->GetData());
        if (posVBO->GetCoordShiftAndScaleEnabled())
        {
          std::vector<double> const& shift = posVBO->GetShift();
          std::vector<double> const& scale = posVBO->GetScale();
          this->VBOInverseTransform->Identity();
          this->VBOInverseTransform->Translate(shift[0], shift[1], shift[2]);
          this->VBOInverseTransform->Scale(1.0 / scale[0], 1.0 / scale[1], 1.0 / scale[2]);
          this->VBOInverseTransform->GetTranspose(this->VBOShiftScale);
        }
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::ComputeBounds()
{
  if (!this->GetInput())
  {
    vtkMath::UninitializeBounds(this->Bounds);
    return;
  }
  this->GetInput()->GetBounds(this->Bounds);
}

//------------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::UpdateBufferObjects(vtkRenderer* ren, vtkActor* act)
{
  // Rebuild buffers if needed
  if (this->GetNeedToRebuildBufferObjects(ren, act))
  {
    this->BuildBufferObjects(ren, act);
  }

  // construct the selection IBO that will reuse the current VBO
  vtkSelection* sel = this->GetSelection();
  if (sel && sel->GetNumberOfNodes() > 0)
  {
    if (sel->GetMTime() > this->SelectionTime)
    {
      std::vector<unsigned int> indexArray[vtkOpenGLPolyDataMapper::PrimitiveTriStrips + 1];
      this->BuildSelectionIBO(this->CurrentInput, indexArray, 0);

      for (vtkIdType p = vtkOpenGLPolyDataMapper::PrimitiveStart;
           p <= vtkOpenGLPolyDataMapper::PrimitiveTriStrips; p++)
      {
        auto& ibo = this->SelectionPrimitives[p].IBO;

        ibo->Upload(indexArray[p], vtkOpenGLIndexBufferObject::ElementArrayBuffer);
        ibo->IndexCount = indexArray[p].size();
      }

      this->SelectionTime = sel->GetMTime();
    }
  }
}

//------------------------------------------------------------------------------
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
      // Allocate memory to allow for faster direct access methods instead of using push_back to
      // populate the array.
      size_t nnSize = newNorms.size(); // Composite mappers can already have values in the array
      newNorms.resize(nnSize + 4 * ccmap->GetSize(), 0.0f);
      for (size_t i = 0; i < ccmap->GetSize(); i++)
      {
        // RGB32F requires a later version of OpenGL than 3.2
        // with 3.2 we know we have RGBA32F hence the extra value
        double* norms = n->GetTuple(ccmap->GetValue(i));
        newNorms[nnSize + i * 4 + 0] = (norms[0]);
        newNorms[nnSize + i * 4 + 1] = (norms[1]);
        newNorms[nnSize + i * 4 + 2] = (norms[2]);
        /* newNorms[nnSize + i * 4 + 3] = (0); */
        // Don't set the final value because it is already set faster by the vector resize above.
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

//------------------------------------------------------------------------------
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
    posVBO->SetProp3D(act);
    posVBO->SetCamera(ren->GetActiveCamera());
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

  this->VBOs->BuildAllVBOs(ren);

  // refetch as it could have been deleted
  posVBO = this->VBOs->GetVBO("vertexMC");
  if (posVBO && posVBO->GetCoordShiftAndScaleEnabled())
  {
    std::vector<double> const& shift = posVBO->GetShift();
    std::vector<double> const& scale = posVBO->GetScale();
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
void vtkOpenGLPolyDataMapper::BuildIBO(vtkRenderer* ren, vtkActor* act, vtkPolyData* poly)
{
  vtkCellArray* prims[4];
  prims[0] = poly->GetVerts();
  prims[1] = poly->GetLines();
  prims[2] = poly->GetPolys();
  prims[3] = poly->GetStrips();
  int representation = act->GetProperty()->GetRepresentation();

  vtkDataArray* ef = poly->GetPointData()->GetAttribute(vtkDataSetAttributes::EDGEFLAG);
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
    this->EdgeValues.clear();

    this->IBOBuildState = this->TempState;
    this->Primitives[PrimitivePoints].IBO->CreatePointIndexBuffer(prims[0]);

    if (representation == VTK_POINTS)
    {
      this->Primitives[PrimitiveLines].IBO->CreatePointIndexBuffer(prims[1]);
      this->Primitives[PrimitiveTris].IBO->CreatePointIndexBuffer(prims[2]);
      this->Primitives[vtkOpenGLPolyDataMapper::PrimitiveTriStrips].IBO->CreatePointIndexBuffer(
        prims[3]);
    }
    else // WIREFRAME OR SURFACE
    {
      this->Primitives[PrimitiveLines].IBO->CreateLineIndexBuffer(prims[1]);

      if (representation == VTK_WIREFRAME)
      {
        if (ef)
        {
          this->Primitives[PrimitiveTris].IBO->CreateEdgeFlagIndexBuffer(prims[2], ef);
        }
        else
        {
          this->Primitives[PrimitiveTris].IBO->CreateTriangleLineIndexBuffer(prims[2]);
        }
        this->Primitives[vtkOpenGLPolyDataMapper::PrimitiveTriStrips].IBO->CreateStripIndexBuffer(
          prims[3], true);
      }
      else // SURFACE
      {
        if (draw_surface_with_edges)
        {
          this->Primitives[PrimitiveTris].IBO->CreateTriangleIndexBuffer(
            prims[2], poly->GetPoints(), &this->EdgeValues, ef);
          if (this->EdgeValues.size())
          {
            if (!this->EdgeTexture)
            {
              this->EdgeTexture = vtkTextureObject::New();
              this->EdgeBuffer = vtkOpenGLBufferObject::New();
              this->EdgeBuffer->SetType(vtkOpenGLBufferObject::TextureBuffer);
            }
            this->EdgeTexture->SetContext(static_cast<vtkOpenGLRenderWindow*>(ren->GetVTKWindow()));
            this->EdgeBuffer->Upload(this->EdgeValues, vtkOpenGLBufferObject::TextureBuffer);
            this->EdgeTexture->CreateTextureBuffer(
              static_cast<unsigned int>(this->EdgeValues.size()), 1, VTK_UNSIGNED_CHAR,
              this->EdgeBuffer);
          }
        }
        else
        {
          this->Primitives[PrimitiveTris].IBO->CreateTriangleIndexBuffer(
            prims[2], poly->GetPoints(), nullptr, nullptr);
        }
        this->Primitives[vtkOpenGLPolyDataMapper::PrimitiveTriStrips].IBO->CreateStripIndexBuffer(
          prims[3], false);
      }
    }

    if (prop->GetVertexVisibility())
    {
      // for all 4 types of primitives add their verts into the IBO
      this->Primitives[PrimitiveVertices].IBO->CreateVertexIndexBuffer(prims);
    }
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::AddPointIdsToSelectionPrimitives(vtkPolyData* poly,
  const char* arrayName, unsigned int processId, unsigned int compositeIndex, vtkIdType selectedId)
{
  // point selection
  auto addPointId = [this](vtkIdType id) {
    for (vtkIdType p = vtkOpenGLPolyDataMapper::PrimitiveStart;
         p <= vtkOpenGLPolyDataMapper::PrimitiveTriStrips; p++)
    {
      this->SelectionArrays[p]->InsertNextCell(1, &id);
    }
  };

  if (arrayName)
  {
    // compute corresponding point ids from selected id or value.
    this->BuildSelectionCache(arrayName, true, poly);
    for (vtkIdType idx :
      this->SelectionCache[std::make_tuple(processId, compositeIndex, selectedId)])
    {
      addPointId(idx);
    }
  }
  else
  {
    addPointId(selectedId);
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::AddCellIdsToSelectionPrimitives(vtkPolyData* poly,
  const char* arrayName, unsigned int processId, unsigned int compositeIndex, vtkIdType selectedId)
{

  auto addCellId = [this, poly](vtkIdType id) {
    vtkIdType npts;
    const vtkIdType* pts;
    vtkIdType nbVerts = poly->GetVerts() ? poly->GetVerts()->GetNumberOfCells() : 0;
    vtkIdType nbLines = poly->GetLines() ? poly->GetLines()->GetNumberOfCells() : 0;
    vtkIdType nbPolys = poly->GetPolys() ? poly->GetPolys()->GetNumberOfCells() : 0;
    vtkIdType nbStrips = poly->GetStrips() ? poly->GetStrips()->GetNumberOfCells() : 0;

    if (poly->GetVerts() && id < nbVerts)
    {
      poly->GetVerts()->GetCellAtId(id, npts, pts);
      this->SelectionArrays[0]->InsertNextCell(npts, pts);
    }
    else if (poly->GetLines() && id < nbVerts + nbLines)
    {
      poly->GetLines()->GetCellAtId(id - nbVerts, npts, pts);
      this->SelectionArrays[1]->InsertNextCell(npts, pts);
    }
    else if (poly->GetPolys() && id < nbVerts + nbLines + nbPolys)
    {
      poly->GetPolys()->GetCellAtId(id - nbVerts - nbLines, npts, pts);
      this->SelectionArrays[2]->InsertNextCell(npts, pts);
    }
    else if (poly->GetStrips() && id < nbVerts + nbLines + nbPolys + nbStrips)
    {
      poly->GetStrips()->GetCellAtId(id - nbVerts - nbLines - nbPolys, npts, pts);
      this->SelectionArrays[3]->InsertNextCell(npts, pts);
    }
  };

  if (arrayName)
  {
    // compute corresponding cell ids from selected id or value.
    this->BuildSelectionCache(arrayName, false, poly);
    for (vtkIdType idx :
      this->SelectionCache[std::make_tuple(processId, compositeIndex, selectedId)])
    {
      addCellId(idx);
    }
  }
  else
  {
    addCellId(selectedId);
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::BuildSelectionIBO(
  vtkPolyData* poly, std::vector<unsigned int> (&indices)[4], vtkIdType offset)
{
  // We need to construct primitives based on a vtkSelection.
  // These primitives are filtered based on composite index and process index.
  for (int i = 0; i < 4; i++)
  {
    this->SelectionArrays[i]->Reset();
  }

  int fieldType = vtkSelectionNode::POINT;
  int contentType = vtkSelectionNode::INDICES;
  for (unsigned int i = 0; i < this->Selection->GetNumberOfNodes(); i++)
  {
    vtkSelectionNode* node = this->Selection->GetNode(i);

    // gather selection types (field type and content type) to determine if the selection
    // is related to point or cell, and if the selection ids are related to a specific
    // array (selection by value) or related directly to polydata ids (selection by id).
    if (i == 0)
    {
      fieldType = node->GetFieldType();
      contentType = node->GetContentType();
    }
    else
    {
      if (fieldType != node->GetFieldType() || contentType != node->GetContentType())
      {
        vtkWarningMacro(
          "All selection nodes must be of the same type. Only the first node will be used.");
        continue;
      }
    }

    // get the process id and the composite id
    vtkInformation* info = node->GetProperties();

    int processId = 0;
    if (info->Has(vtkSelectionNode::PROCESS_ID()))
    {
      processId = info->Get(vtkSelectionNode::PROCESS_ID());
    }
    int compositeIndex = 0;
    if (info->Has(vtkSelectionNode::COMPOSITE_INDEX()))
    {
      compositeIndex = info->Get(vtkSelectionNode::COMPOSITE_INDEX());
    }

    vtkDataSetAttributes* attr = node->GetSelectionData();
    for (vtkIdType j = 0; j < attr->GetNumberOfArrays(); j++)
    {
      vtkIdTypeArray* idArray = vtkIdTypeArray::SafeDownCast(attr->GetArray(j));
      if (idArray)
      {
        // determine the name of the array to use
        const char* arrayName = nullptr;
        if (contentType == vtkSelectionNode::SelectionContent::VALUES)
        {
          arrayName = idArray->GetName();
        }
        else if (contentType == vtkSelectionNode::SelectionContent::INDICES)
        {
          arrayName = fieldType == vtkSelectionNode::SelectionField::POINT ? this->PointIdArrayName
                                                                           : this->CellIdArrayName;
        }

        // for each selected id, add the corresponding local id(s).
        // it can be different if selection by value is enabled or if a process id or composite id
        // is defined.
        for (vtkIdType k = 0; k < idArray->GetNumberOfTuples(); k++)
        {
          vtkIdType selectedId = idArray->GetTypedComponent(k, 0);

          if (fieldType == vtkSelectionNode::SelectionField::POINT)
          {
            this->AddPointIdsToSelectionPrimitives(
              poly, arrayName, processId, compositeIndex, selectedId);
          }
          else
          {
            this->AddCellIdsToSelectionPrimitives(
              poly, arrayName, processId, compositeIndex, selectedId);
          }
        }
      }
    }
  }

  // build OpenGL IBO from vtkCellArray list
  this->SelectionPrimitives[PrimitivePoints].IBO->AppendPointIndexBuffer(
    indices[0], this->SelectionArrays[0], offset);

  if (fieldType == vtkSelectionNode::SelectionField::POINT)
  {
    this->SelectionPrimitives[PrimitiveLines].IBO->AppendPointIndexBuffer(
      indices[1], this->SelectionArrays[1], offset);
    this->SelectionPrimitives[PrimitiveTris].IBO->AppendPointIndexBuffer(
      indices[2], this->SelectionArrays[2], offset);
    this->SelectionPrimitives[vtkOpenGLPolyDataMapper::PrimitiveTriStrips]
      .IBO->AppendPointIndexBuffer(indices[3], this->SelectionArrays[3], offset);
    this->SelectionType = VTK_POINTS;
  }
  else
  {
    // Cell selection is always represented using wireframe
    this->SelectionPrimitives[PrimitiveLines].IBO->AppendLineIndexBuffer(
      indices[1], this->SelectionArrays[1], offset);
    this->SelectionPrimitives[PrimitiveTris].IBO->AppendTriangleLineIndexBuffer(
      indices[2], this->SelectionArrays[2], offset);
    this->SelectionPrimitives[vtkOpenGLPolyDataMapper::PrimitiveTriStrips]
      .IBO->AppendStripIndexBuffer(indices[3], this->SelectionArrays[3], offset, true);
    this->SelectionType = VTK_WIREFRAME;
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::BuildSelectionCache(
  const char* arrayName, bool selectingPoints, vtkPolyData* poly)
{
  if (arrayName &&
    (this->SelectionCacheForPoints != selectingPoints || this->SelectionCacheName != arrayName ||
      this->SelectionCacheTime < poly->GetMTime() || poly != this->SelectionPolyData))
  {
    // the cache needs a rebuild
    this->SelectionCache.clear();

    vtkDataSetAttributes* attr = selectingPoints
      ? static_cast<vtkDataSetAttributes*>(poly->GetPointData())
      : static_cast<vtkDataSetAttributes*>(poly->GetCellData());

    vtkIdTypeArray* idArray = vtkIdTypeArray::SafeDownCast(attr->GetArray(arrayName));
    vtkUnsignedIntArray* compArray =
      vtkUnsignedIntArray::SafeDownCast(attr->GetArray(this->CompositeIdArrayName));
    vtkUnsignedIntArray* procArray =
      vtkUnsignedIntArray::SafeDownCast(attr->GetArray(this->ProcessIdArrayName));

    // a selection cache is built here to map a tuple (process id, composite id, value id) to the
    // the selected id. This will speed up look-ups at runtime.
    if (idArray && idArray->GetNumberOfComponents() == 1)
    {
      for (vtkIdType i = 0; i < idArray->GetNumberOfTuples(); i++)
      {
        vtkIdType val = idArray->GetTypedComponent(i, 0);
        unsigned int procId = procArray ? procArray->GetTypedComponent(i, 0) : 0;
        unsigned int compIndex = compArray ? compArray->GetTypedComponent(i, 0) : 0;

        this->SelectionCache[std::make_tuple(procId, compIndex, val)].push_back(i);
      }
    }

    this->SelectionCacheForPoints = selectingPoints;
    this->SelectionCacheName = arrayName;
    this->SelectionCacheTime = poly->GetMTime();
    this->SelectionPolyData = poly;
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::ShallowCopy(vtkAbstractMapper* mapper)
{
  vtkOpenGLPolyDataMapper* m = vtkOpenGLPolyDataMapper::SafeDownCast(mapper);
  if (m != nullptr)
  {
    this->SetPointIdArrayName(m->GetPointIdArrayName());
    this->SetCompositeIdArrayName(m->GetCompositeIdArrayName());
    this->SetProcessIdArrayName(m->GetProcessIdArrayName());
    this->SetCellIdArrayName(m->GetCellIdArrayName());
    this->SetVertexShaderCode(m->GetVertexShaderCode());
    this->SetGeometryShaderCode(m->GetGeometryShaderCode());
    this->SetFragmentShaderCode(m->GetFragmentShaderCode());
  }

  // Now do superclass
  this->vtkPolyDataMapper::ShallowCopy(mapper);
}

void vtkOpenGLPolyDataMapper::SetVBOShiftScaleMethod(int m)
{
  if (this->ShiftScaleMethod == m)
  {
    return;
  }

  this->ShiftScaleMethod = m;
  vtkOpenGLVertexBufferObject* posVBO = this->VBOs->GetVBO("vertexMC");
  if (posVBO)
  {
    posVBO->SetCoordShiftAndScaleMethod(
      static_cast<vtkOpenGLVertexBufferObject::ShiftScaleMethod>(this->ShiftScaleMethod));
  }
}

int vtkOpenGLPolyDataMapper::GetOpenGLMode(int representation, int primType)
{
  if (representation == VTK_POINTS || primType == PrimitivePoints || primType == PrimitiveVertices)
  {
    return GL_POINTS;
  }
  if (representation == VTK_WIREFRAME || primType == PrimitiveLines)
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

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::RemoveAllVertexAttributeMappings()
{
  for (auto itr = this->ExtraAttributes.begin(); itr != this->ExtraAttributes.end();
       itr = this->ExtraAttributes.begin())
  {
    this->RemoveVertexAttributeMapping(itr->first.c_str());
  }
}

//------------------------------------------------------------------------------
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
        vtkIdType outval = pointArrayId->GetValue(inval);
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
        vtkIdType outval = pointArrayId->GetValue(inval);
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
        vtkIdType vtkCellId =
          this->CellCellMap->ConvertOpenGLCellIdToVTKCellId(this->PointPicking, inval);
        unsigned int outval = compositeArray->GetValue(vtkCellId);
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
        vtkIdType outval =
          this->CellCellMap->ConvertOpenGLCellIdToVTKCellId(this->PointPicking, inval);
        if (cellArrayId)
        {
          outval = cellArrayId->GetValue(outval);
        }
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
        vtkIdType outval =
          this->CellCellMap->ConvertOpenGLCellIdToVTKCellId(this->PointPicking, inval);
        if (cellArrayId)
        {
          outval = cellArrayId->GetValue(outval);
        }
        chighdata[pos] = (outval & 0xff000000) >> 24;
        chighdata[pos + 1] = (outval & 0xff00000000) >> 32;
        chighdata[pos + 2] = (outval & 0xff0000000000) >> 40;
      }
    }
  }
}
