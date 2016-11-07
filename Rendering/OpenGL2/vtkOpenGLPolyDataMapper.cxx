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
#include "vtkOpenGLError.h"
#include "vtkOpenGLHelper.h"
#include "vtkOpenGLIndexBufferObject.h"
#include "vtkOpenGLRenderPass.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLResourceFreeCallback.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLTexture.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkOpenGLVertexBufferObject.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkScalarsToColors.h"
#include "vtkShaderProgram.h"
#include "vtkTextureObject.h"
#include "vtkTransform.h"
#include "vtkUnsignedIntArray.h"
#if GL_ES_VERSION_2_0 != 1 && GL_ES_VERSION_3_0 != 1
#include "vtkValuePass.h"
#include "vtkValuePassHelper.h"
#endif
#include "vtkShadowMapPass.h"

// Bring in our fragment lit shader symbols.
#include "vtkPolyDataVS.h"
#include "vtkPolyDataFS.h"
#include "vtkPolyDataWideLineGS.h"

#include <algorithm>
#include <sstream>


//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLPolyDataMapper)

//-----------------------------------------------------------------------------
vtkOpenGLPolyDataMapper::vtkOpenGLPolyDataMapper()
  : UsingScalarColoring(false)
{
  this->InternalColorTexture = 0;
  this->PopulateSelectionSettings = 1;
  this->LastSelectionState = vtkHardwareSelector::MIN_KNOWN_PASS - 1;
  this->CurrentInput = 0;
  this->TempMatrix4 = vtkMatrix4x4::New();
  this->TempMatrix3 = vtkMatrix3x3::New();
  this->DrawingEdges = false;
  this->ForceTextureCoordinates = false;

  this->PrimitiveIDOffset = 0;

  this->CellScalarTexture = NULL;
  this->CellScalarBuffer = NULL;
  this->CellNormalTexture = NULL;
  this->CellNormalBuffer = NULL;

  this->HavePickScalars = false;
  this->HaveCellScalars = false;
  this->HaveCellNormals = false;

  this->PointIdArrayName = NULL;
  this->CellIdArrayName = NULL;
  this->ProcessIdArrayName = NULL;
  this->CompositeIdArrayName = NULL;
  this->VBO = vtkOpenGLVertexBufferObject::New();

  this->AppleBugPrimIDBuffer = 0;
  this->HaveAppleBug = false;
  this->HaveAppleBugForce = 0;
  this->LastBoundBO = NULL;

  this->VertexShaderCode = 0;
  this->FragmentShaderCode = 0;
  this->GeometryShaderCode = 0;

  this->LastLightComplexity[&this->Points] = -1;
  this->LastLightComplexity[&this->Lines] = -1;
  this->LastLightComplexity[&this->Tris] = -1;
  this->LastLightComplexity[&this->TriStrips] = -1;
  this->LastLightComplexity[&this->TrisEdges] = -1;
  this->LastLightComplexity[&this->TriStripsEdges] = -1;

  this->TimerQuery = 0;
  this->ResourceCallback = new vtkOpenGLResourceFreeCallback<vtkOpenGLPolyDataMapper>(this,
    &vtkOpenGLPolyDataMapper::ReleaseGraphicsResources);
#if GL_ES_VERSION_2_0 != 1 && GL_ES_VERSION_3_0 != 1
  this->ValuePassHelper = vtkSmartPointer<vtkValuePassHelper>::New();
#endif
}

//-----------------------------------------------------------------------------
vtkOpenGLPolyDataMapper::~vtkOpenGLPolyDataMapper()
{
  if (this->ResourceCallback)
  {
    this->ResourceCallback->Release();
    delete this->ResourceCallback;
    this->ResourceCallback = NULL;
  }
  if (this->InternalColorTexture)
  { // Resources released previously.
    this->InternalColorTexture->Delete();
    this->InternalColorTexture = 0;
  }
  this->TempMatrix3->Delete();
  this->TempMatrix4->Delete();

  if (this->CellScalarTexture)
  { // Resources released previously.
    this->CellScalarTexture->Delete();
    this->CellScalarTexture = 0;
  }
  if (this->CellScalarBuffer)
  { // Resources released previously.
    this->CellScalarBuffer->Delete();
    this->CellScalarBuffer = 0;
  }

  if (this->CellNormalTexture)
  { // Resources released previously.
    this->CellNormalTexture->Delete();
    this->CellNormalTexture = 0;
  }
  if (this->CellNormalBuffer)
  { // Resources released previously.
    this->CellNormalBuffer->Delete();
    this->CellNormalBuffer = 0;
  }

  this->SetPointIdArrayName(NULL);
  this->SetCellIdArrayName(NULL);
  this->SetProcessIdArrayName(NULL);
  this->SetCompositeIdArrayName(NULL);
  this->VBO->Delete();
  this->VBO = 0;

  if (this->AppleBugPrimIDBuffer)
  {
    this->AppleBugPrimIDBuffer->Delete();
  }

  this->SetVertexShaderCode(0);
  this->SetFragmentShaderCode(0);
  this->SetGeometryShaderCode(0);
}

//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::ReleaseGraphicsResources(vtkWindow* win)
{
  if (!this->ResourceCallback->IsReleasing())
  {
    this->ResourceCallback->Release();
    return;
  }

  this->VBO->ReleaseGraphicsResources();
  this->Points.ReleaseGraphicsResources(win);
  this->Lines.ReleaseGraphicsResources(win);
  this->Tris.ReleaseGraphicsResources(win);
  this->TriStrips.ReleaseGraphicsResources(win);
  this->TrisEdges.ReleaseGraphicsResources(win);
  this->TriStripsEdges.ReleaseGraphicsResources(win);

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

#if GL_ES_VERSION_2_0 != 1 && GL_ES_VERSION_3_0 != 1
  this->ValuePassHelper->ReleaseGraphicsResources(win);
#endif

  if (this->AppleBugPrimIDBuffer)
  {
    this->AppleBugPrimIDBuffer->ReleaseGraphicsResources();
  }
  if (this->TimerQuery)
  {
#if GL_ES_VERSION_2_0 != 1 && GL_ES_VERSION_3_0 != 1
    glDeleteQueries(1, &this->TimerQuery);
#endif
    this->TimerQuery = 0;
  }
  this->VBOBuildString = "";
  this->IBOBuildString = "";
  this->Modified();
}

void vtkOpenGLPolyDataMapper::AddShaderReplacement(
    vtkShader::Type shaderType, // vertex, fragment, etc
    std::string originalValue,
    bool replaceFirst,  // do this replacement before the default
    std::string replacementValue,
    bool replaceAll)
{
  vtkOpenGLPolyDataMapper::ReplacementSpec spec;
  spec.ShaderType = shaderType;
  spec.OriginalValue = originalValue;
  spec.ReplaceFirst = replaceFirst;

  vtkOpenGLPolyDataMapper::ReplacementValue values;
  values.Replacement = replacementValue;
  values.ReplaceAll = replaceAll;

  this->UserShaderReplacements[spec] = values;
}

void vtkOpenGLPolyDataMapper::ClearShaderReplacement(
    vtkShader::Type shaderType, // vertex, fragment, etc
    std::string originalValue,
    bool replaceFirst)
{
  vtkOpenGLPolyDataMapper::ReplacementSpec spec;
  spec.ShaderType = shaderType;
  spec.OriginalValue = originalValue;
  spec.ReplaceFirst = replaceFirst;

  typedef std::map<const vtkOpenGLPolyDataMapper::ReplacementSpec,
    vtkOpenGLPolyDataMapper::ReplacementValue>::iterator RIter;
  RIter found = this->UserShaderReplacements.find(spec);
  if (found == this->UserShaderReplacements.end())
  {
    this->UserShaderReplacements.erase(found);
  }
}

//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::BuildShaders(
    std::map<vtkShader::Type, vtkShader *> shaders,
    vtkRenderer *ren, vtkActor *actor)
{
  this->GetShaderTemplate(shaders, ren, actor);

  typedef std::map<const vtkOpenGLPolyDataMapper::ReplacementSpec,
    vtkOpenGLPolyDataMapper::ReplacementValue>::const_iterator RIter;

  // user specified pre replacements
  for (RIter i = this->UserShaderReplacements.begin();
    i != this->UserShaderReplacements.end(); i++)
  {
    if (i->first.ReplaceFirst)
    {
      std::string ssrc = shaders[i->first.ShaderType]->GetSource();
      vtkShaderProgram::Substitute(ssrc,
        i->first.OriginalValue,
        i->second.Replacement,
        i->second.ReplaceAll);
      shaders[i->first.ShaderType]->SetSource(ssrc);
    }
  }

  this->ReplaceShaderValues(shaders, ren, actor);

  // user specified post replacements
  for (RIter i = this->UserShaderReplacements.begin();
    i != this->UserShaderReplacements.end(); i++)
  {
    if (!i->first.ReplaceFirst)
    {
      std::string ssrc = shaders[i->first.ShaderType]->GetSource();
      vtkShaderProgram::Substitute(ssrc,
        i->first.OriginalValue,
        i->second.Replacement,
        i->second.ReplaceAll);
      shaders[i->first.ShaderType]->SetSource(ssrc);
    }
  }
}

//-----------------------------------------------------------------------------
bool vtkOpenGLPolyDataMapper::HaveWideLines(
  vtkRenderer *ren,
  vtkActor *actor)
{
  if ((this->LastBoundBO == &this->Lines ||
       this->LastBoundBO == &this->TrisEdges ||
       this->LastBoundBO == &TriStripsEdges ||
       (actor->GetProperty()->GetRepresentation() == VTK_WIREFRAME &&
          (this->LastBoundBO == &this->Tris ||
            this->LastBoundBO == &this->TriStrips)))
      && actor->GetProperty()->GetLineWidth() > 1.0
      && vtkOpenGLRenderWindow::GetContextSupportsOpenGL32())
  {
    // we have wide lines, but the OpenGL implementation may
    // actually support them, check the range to see if we
      // really need have to implement our own wide lines
    vtkOpenGLRenderWindow *renWin = vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());
    return actor->GetProperty()->GetRenderLinesAsTubes() || !(renWin &&
      renWin->GetMaximumHardwareLineWidth() >= actor->GetProperty()->GetLineWidth());
  }
  return false;
}

//-----------------------------------------------------------------------------
vtkMTimeType vtkOpenGLPolyDataMapper::GetRenderPassStageMTime(vtkActor *actor)
{
  vtkInformation *info = actor->GetPropertyKeys();
  vtkMTimeType renderPassMTime = 0;

  int curRenderPasses = 0;
  if (info && info->Has(vtkOpenGLRenderPass::RenderPasses()))
  {
    curRenderPasses = info->Length(vtkOpenGLRenderPass::RenderPasses());
  }

  int lastRenderPasses = 0;
  if (this->LastRenderPassInfo->Has(vtkOpenGLRenderPass::RenderPasses()))
  {
    lastRenderPasses =
        this->LastRenderPassInfo->Length(vtkOpenGLRenderPass::RenderPasses());
  }

  // Determine the last time a render pass changed stages:
  if (curRenderPasses != lastRenderPasses)
  {
    // Number of passes changed, definitely need to update.
    // Fake the time to force an update:
    renderPassMTime = VTK_UNSIGNED_LONG_MAX;
  }
  else
  {
    // Compare the current to the previous render passes:
    for (int i = 0; i < curRenderPasses; ++i)
    {
      vtkObjectBase *curRP = info->Get(vtkOpenGLRenderPass::RenderPasses(), i);
      vtkObjectBase *lastRP =
          this->LastRenderPassInfo->Get(vtkOpenGLRenderPass::RenderPasses(), i);

      if (curRP != lastRP)
      {
        // Render passes have changed. Force update:
        renderPassMTime = VTK_UNSIGNED_LONG_MAX;
        break;
      }
      else
      {
        // Render passes have not changed -- check MTime.
        vtkOpenGLRenderPass *rp = static_cast<vtkOpenGLRenderPass*>(curRP);
        renderPassMTime = std::max(renderPassMTime, rp->GetShaderStageMTime());
      }
    }
  }

  // Cache the current set of render passes for next time:
  if (info)
  {
    this->LastRenderPassInfo->CopyEntry(info,
                                        vtkOpenGLRenderPass::RenderPasses());
  }
  else
  {
    this->LastRenderPassInfo->Clear();
  }

  return renderPassMTime;
}

//-----------------------------------------------------------------------------
bool vtkOpenGLPolyDataMapper::HaveTextures(vtkActor *actor)
{
  return (this->GetNumberOfTextures(actor) > 0);
}

//-----------------------------------------------------------------------------
unsigned int vtkOpenGLPolyDataMapper::GetNumberOfTextures(vtkActor *actor)
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
std::vector<vtkTexture *> vtkOpenGLPolyDataMapper::GetTextures(vtkActor *actor)
{
  std::vector<vtkTexture *> res;

  if (this->ColorTextureMap)
  {
    res.push_back(this->InternalColorTexture);
  }
  if (actor->GetTexture())
  {
    res.push_back(actor->GetTexture());
  }
  for (int i = 0; i < actor->GetProperty()->GetNumberOfTextures(); i++)
  {
    res.push_back(actor->GetProperty()->GetTexture(i));
  }
  return res;
}

//-----------------------------------------------------------------------------
bool vtkOpenGLPolyDataMapper::HaveTCoords(vtkPolyData *poly)
{
  return (this->ColorCoordinates ||
          poly->GetPointData()->GetTCoords() ||
          this->ForceTextureCoordinates);
}

//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::GetShaderTemplate(
    std::map<vtkShader::Type, vtkShader *> shaders,
    vtkRenderer *ren, vtkActor *actor)
{
  if (this->VertexShaderCode && strcmp(this->VertexShaderCode,"") != 0)
  {
    shaders[vtkShader::Vertex]->SetSource(this->VertexShaderCode);
  }
  else
  {
    shaders[vtkShader::Vertex]->SetSource(vtkPolyDataVS);
  }

  if (this->FragmentShaderCode && strcmp(this->FragmentShaderCode,"") != 0)
  {
    shaders[vtkShader::Fragment]->SetSource(this->FragmentShaderCode);
  }
  else
  {
    shaders[vtkShader::Fragment]->SetSource(vtkPolyDataFS);
  }

  if (this->GeometryShaderCode && strcmp(this->GeometryShaderCode,"") != 0)
  {
    shaders[vtkShader::Geometry]->SetSource(this->GeometryShaderCode);
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
    std::map<vtkShader::Type, vtkShader *> shaders, vtkRenderer *,
    vtkActor *act)
{
  std::string VSSource = shaders[vtkShader::Vertex]->GetSource();
  std::string GSSource = shaders[vtkShader::Geometry]->GetSource();
  std::string FSSource = shaders[vtkShader::Fragment]->GetSource();

  vtkInformation *info = act->GetPropertyKeys();
  if (info && info->Has(vtkOpenGLRenderPass::RenderPasses()))
  {
    int numRenderPasses = info->Length(vtkOpenGLRenderPass::RenderPasses());
    for (int i = 0; i < numRenderPasses; ++i)
    {
      vtkObjectBase *rpBase = info->Get(vtkOpenGLRenderPass::RenderPasses(), i);
      vtkOpenGLRenderPass *rp = static_cast<vtkOpenGLRenderPass*>(rpBase);
      if (!rp->ReplaceShaderValues(VSSource, GSSource, FSSource, this, act))
      {
        vtkErrorMacro("vtkOpenGLRenderPass::ReplaceShaderValues failed for "
                      << rp->GetClassName());
      }
    }
  }

  shaders[vtkShader::Vertex]->SetSource(VSSource);
  shaders[vtkShader::Geometry]->SetSource(GSSource);
  shaders[vtkShader::Fragment]->SetSource(FSSource);
}

//------------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::ReplaceShaderColor(
  std::map<vtkShader::Type, vtkShader *> shaders,
  vtkRenderer *, vtkActor *actor)
{
  std::string VSSource = shaders[vtkShader::Vertex]->GetSource();
  std::string GSSource = shaders[vtkShader::Geometry]->GetSource();
  std::string FSSource = shaders[vtkShader::Fragment]->GetSource();

  // create the material/color property declarations, and VS implementation
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
  if (this->LastLightComplexity[this->LastBoundBO])
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
  if (this->VBO->ColorComponents != 0 && !this->DrawingEdges)
  {
    colorDec += "varying vec4 vertexColorVSOutput;\n";
    vtkShaderProgram::Substitute(VSSource,"//VTK::Color::Dec",
                        "attribute vec4 scalarColor;\n"
                        "varying vec4 vertexColorVSOutput;");
    vtkShaderProgram::Substitute(VSSource,"//VTK::Color::Impl",
                        "vertexColorVSOutput =  scalarColor;");
    vtkShaderProgram::Substitute(GSSource,
      "//VTK::Color::Dec",
      "in vec4 vertexColorVSOutput[];\n"
      "out vec4 vertexColorGSOutput;");
    vtkShaderProgram::Substitute(GSSource,
      "//VTK::Color::Impl",
      "vertexColorGSOutput = vertexColorVSOutput[i];");
  }
  if (this->HaveCellScalars && !this->HavePickScalars && !this->DrawingEdges)
  {
    colorDec += "uniform samplerBuffer textureC;\n";
  }

  vtkShaderProgram::Substitute(FSSource,"//VTK::Color::Dec", colorDec);

  // now handle the more complex fragment shader implementation
  // the following are always defined variables.  We start
  // by assiging a default value from the uniform
  std::string colorImpl =
    "  vec3 ambientColor;\n"
    "  vec3 diffuseColor;\n"
    "  float opacity;\n";

#if GL_ES_VERSION_2_0 != 1 && GL_ES_VERSION_3_0 != 1
  if (this->ValuePassHelper->GetRenderingMode() == vtkValuePass::FLOATING_POINT)
  {
    this->ValuePassHelper->UpdateShaders(VSSource, FSSource, colorImpl);
  }
#endif

  if (this->LastLightComplexity[this->LastBoundBO])
  {
    colorImpl +=
      "  vec3 specularColor;\n"
      "  float specularPower;\n";
  }
  if (actor->GetBackfaceProperty() && !this->DrawingEdges)
  {
    if (this->LastLightComplexity[this->LastBoundBO])
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
    if (this->LastLightComplexity[this->LastBoundBO])
    {
      colorImpl +=
        "  specularColor = specularColorUniform;\n"
        "  specularPower = specularPowerUniform;\n";
    }
  }

  // now handle scalar coloring
  if (this->VBO->ColorComponents != 0 && !this->DrawingEdges)
  {
    if (this->ScalarMaterialMode == VTK_MATERIALMODE_AMBIENT ||
        (this->ScalarMaterialMode == VTK_MATERIALMODE_DEFAULT &&
          actor->GetProperty()->GetAmbient() > actor->GetProperty()->GetDiffuse()))
    {
      vtkShaderProgram::Substitute(FSSource,"//VTK::Color::Impl",
        colorImpl +
        "  ambientColor = vertexColorVSOutput.rgb;\n"
        "  opacity = opacity*vertexColorVSOutput.a;");
    }
    else if (this->ScalarMaterialMode == VTK_MATERIALMODE_DIFFUSE ||
        (this->ScalarMaterialMode == VTK_MATERIALMODE_DEFAULT &&
          actor->GetProperty()->GetAmbient() <= actor->GetProperty()->GetDiffuse()))
    {
      vtkShaderProgram::Substitute(FSSource,"//VTK::Color::Impl", colorImpl +
        "  diffuseColor = vertexColorVSOutput.rgb;\n"
        "  opacity = opacity*vertexColorVSOutput.a;");
    }
    else
    {
      vtkShaderProgram::Substitute(FSSource,"//VTK::Color::Impl", colorImpl +
        "  diffuseColor = vertexColorVSOutput.rgb;\n"
        "  ambientColor = vertexColorVSOutput.rgb;\n"
        "  opacity = opacity*vertexColorVSOutput.a;");
    }
  }
  else
  {
    // are we doing scalar coloring by texture?
    if (this->InterpolateScalarsBeforeMapping &&
        this->ColorCoordinates &&
        !this->DrawingEdges)
    {
      if (this->ScalarMaterialMode == VTK_MATERIALMODE_AMBIENT ||
          (this->ScalarMaterialMode == VTK_MATERIALMODE_DEFAULT &&
            actor->GetProperty()->GetAmbient() > actor->GetProperty()->GetDiffuse()))
      {
        vtkShaderProgram::Substitute(FSSource,
          "//VTK::Color::Impl", colorImpl +
          "  vec4 texColor = texture2D(texture_0, tcoordVCVSOutput.st);\n"
          "  ambientColor = texColor.rgb;\n"
          "  opacity = opacity*texColor.a;");
      }
      else if (this->ScalarMaterialMode == VTK_MATERIALMODE_DIFFUSE ||
          (this->ScalarMaterialMode == VTK_MATERIALMODE_DEFAULT &&
           actor->GetProperty()->GetAmbient() <= actor->GetProperty()->GetDiffuse()))
      {
        vtkShaderProgram::Substitute(FSSource,
          "//VTK::Color::Impl", colorImpl +
          "  vec4 texColor = texture2D(texture_0, tcoordVCVSOutput.st);\n"
          "  diffuseColor = texColor.rgb;\n"
          "  opacity = opacity*texColor.a;");
      }
      else
      {
        vtkShaderProgram::Substitute(FSSource,
          "//VTK::Color::Impl", colorImpl +
          "vec4 texColor = texture2D(texture_0, tcoordVCVSOutput.st);\n"
          "  ambientColor = texColor.rgb;\n"
          "  diffuseColor = texColor.rgb;\n"
          "  opacity = opacity*texColor.a;");
      }
    }
    else
    {
      if (this->HaveCellScalars && !this->DrawingEdges)
      {
        if (this->ScalarMaterialMode == VTK_MATERIALMODE_AMBIENT ||
            (this->ScalarMaterialMode == VTK_MATERIALMODE_DEFAULT &&
              actor->GetProperty()->GetAmbient() > actor->GetProperty()->GetDiffuse()))
        {
          vtkShaderProgram::Substitute(FSSource,
            "//VTK::Color::Impl", colorImpl +
            "  vec4 texColor = texelFetchBuffer(textureC, gl_PrimitiveID + PrimitiveIDOffset);\n"
            "  ambientColor = texColor.rgb;\n"
            "  opacity = opacity*texColor.a;"
            );
        }
        else if (this->ScalarMaterialMode == VTK_MATERIALMODE_DIFFUSE ||
            (this->ScalarMaterialMode == VTK_MATERIALMODE_DEFAULT &&
             actor->GetProperty()->GetAmbient() <= actor->GetProperty()->GetDiffuse()))
        {
          vtkShaderProgram::Substitute(FSSource,
            "//VTK::Color::Impl", colorImpl +
           "  vec4 texColor = texelFetchBuffer(textureC, gl_PrimitiveID + PrimitiveIDOffset);\n"
            "  diffuseColor = texColor.rgb;\n"
            "  opacity = opacity*texColor.a;"
            //  "  diffuseColor = vec3((gl_PrimitiveID%256)/255.0,((gl_PrimitiveID/256)%256)/255.0,1.0);\n"
            );
        }
        else
        {
          vtkShaderProgram::Substitute(FSSource,
            "//VTK::Color::Impl", colorImpl +
            "vec4 texColor = texelFetchBuffer(textureC, gl_PrimitiveID + PrimitiveIDOffset);\n"
            "  ambientColor = texColor.rgb;\n"
            "  diffuseColor = texColor.rgb;\n"
            "  opacity = opacity*texColor.a;"
            );
        }
      }
      vtkShaderProgram::Substitute(FSSource,"//VTK::Color::Impl", colorImpl);
    }
  }

  shaders[vtkShader::Vertex]->SetSource(VSSource);
  shaders[vtkShader::Geometry]->SetSource(GSSource);
  shaders[vtkShader::Fragment]->SetSource(FSSource);
}

void vtkOpenGLPolyDataMapper::ReplaceShaderLight(
  std::map<vtkShader::Type, vtkShader *> shaders,
  vtkRenderer *, vtkActor *actor)
{
  std::string FSSource = shaders[vtkShader::Fragment]->GetSource();

  // check for normal rendering
  vtkInformation *info = actor->GetPropertyKeys();
  if (info && info->Has(vtkLightingMapPass::RENDER_NORMALS()))
  {
      vtkShaderProgram::Substitute(FSSource,"//VTK::Light::Impl",
        "  vec3 n = (normalVCVSOutput + 1.0) * 0.5;\n"
        "  gl_FragData[0] = vec4(n.x, n.y, n.z, 1.0);"
      );
      shaders[vtkShader::Fragment]->SetSource(FSSource);
      return;
  }

  // check for shadow maps
  std::string shadowFactor = "";
  if (info && info->Has(vtkShadowMapPass::ShadowMapPass()))
  {
    vtkShadowMapPass *smp = vtkShadowMapPass::SafeDownCast(
      info->Get(vtkShadowMapPass::ShadowMapPass()));
    if (smp)
    {
      vtkShaderProgram::Substitute(FSSource,"//VTK::Light::Dec",
        smp->GetFragmentDeclaration(), false);
      vtkShaderProgram::Substitute(FSSource,"//VTK::Light::Impl",
        smp->GetFragmentImplementation(), false);
      shadowFactor = "*factors[lightNum]";
    }
  }

  // If rendering, set diffuse and specular colors to pure white
  if (info && info->Has(vtkLightingMapPass::RENDER_LUMINANCE()))
  {
      vtkShaderProgram::Substitute(FSSource, "//VTK::Light::Impl",
        "  diffuseColor = vec3(1, 1, 1);\n"
        "  specularColor = vec3(1, 1, 1);\n"
        "  //VTK::Light::Impl\n",
        false
      );
  }

  int lastLightComplexity = this->LastLightComplexity[this->LastBoundBO];
#if GL_ES_VERSION_2_0 != 1 && GL_ES_VERSION_3_0 != 1
  if (info && info->Has(vtkValuePass::RENDER_VALUES()))
  {
    // Although vtkValuePass::FLOATING_POINT does not require this, it is for
    // simplicity left unchanged (only required when using INVERTIBLE_LUT mode).
    lastLightComplexity = 0;
  }
#endif

  switch (lastLightComplexity)
  {
    case 0: // no lighting or RENDER_VALUES
      vtkShaderProgram::Substitute(FSSource, "//VTK::Light::Impl",
        "  gl_FragData[0] = vec4(ambientColor + diffuseColor, opacity);\n"
        "  //VTK::Light::Impl\n",
        false
        );
      break;

    case 1:  // headlight
      vtkShaderProgram::Substitute(FSSource, "//VTK::Light::Impl",
        "  float df = max(0.0, normalVCVSOutput.z);\n"
        "  float sf = pow(df, specularPower);\n"
        "  vec3 diffuse = df * diffuseColor;\n"
        "  vec3 specular = sf * specularColor;\n"
        "  gl_FragData[0] = vec4(ambientColor + diffuse + specular, opacity);\n"
        "  //VTK::Light::Impl\n",
        false
      );
      break;

    case 2: // light kit
      vtkShaderProgram::Substitute(FSSource,"//VTK::Light::Dec",
        // only allow for up to 6 active lights
        "uniform int numberOfLights;\n"
        // intensity weighted color
        "uniform vec3 lightColor[6];\n"
        "uniform vec3 lightDirectionVC[6]; // normalized\n"
        "uniform vec3 lightHalfAngleVC[6]; // normalized"
      );
      vtkShaderProgram::Substitute(FSSource,"//VTK::Light::Impl",
        "vec3 diffuse = vec3(0,0,0);\n"
        "  vec3 specular = vec3(0,0,0);\n"
        "  for (int lightNum = 0; lightNum < numberOfLights; lightNum++)\n"
        "    {\n"
        "    float df = max(0.0, dot(normalVCVSOutput, -lightDirectionVC[lightNum]));\n"
        "    diffuse += ((df" + shadowFactor + ") * lightColor[lightNum]);\n"
        "    if (dot(normalVCVSOutput, lightDirectionVC[lightNum]) < 0.0)\n"
        "      {\n"
        "      float sf = pow( max(0.0, dot(lightHalfAngleVC[lightNum],normalVCVSOutput)), specularPower);\n"
        "      specular += ((sf" + shadowFactor + ") * lightColor[lightNum]);\n"
        "      }\n"
        "    }\n"
        "  diffuse = diffuse * diffuseColor;\n"
        "  specular = specular * specularColor;\n"
        "  gl_FragData[0] = vec4(ambientColor + diffuse + specular, opacity);"
        "  //VTK::Light::Impl",
        false
      );
      break;

    case 3: // positional
      vtkShaderProgram::Substitute(FSSource,"//VTK::Light::Dec",
        // only allow for up to 6 active lights
        "uniform int numberOfLights;\n"
        // intensity weighted color
        "uniform vec3 lightColor[6];\n"
        "uniform vec3 lightDirectionVC[6]; // normalized\n"
        "uniform vec3 lightHalfAngleVC[6]; // normalized\n"
        "uniform vec3 lightPositionVC[6];\n"
        "uniform vec3 lightAttenuation[6];\n"
        "uniform float lightConeAngle[6];\n"
        "uniform float lightExponent[6];\n"
        "uniform int lightPositional[6];"
      );
      vtkShaderProgram::Substitute(FSSource,"//VTK::Light::Impl",
        "  vec3 diffuse = vec3(0,0,0);\n"
        "  vec3 specular = vec3(0,0,0);\n"
        "  vec3 vertLightDirectionVC;\n"
        "  for (int lightNum = 0; lightNum < numberOfLights; lightNum++)\n"
        "    {\n"
        "    float attenuation = 1.0;\n"
        "    if (lightPositional[lightNum] == 0)\n"
        "      {\n"
        "      vertLightDirectionVC = lightDirectionVC[lightNum];\n"
        "      }\n"
        "    else\n"
        "      {\n"
        "      vertLightDirectionVC = vertexVC.xyz - lightPositionVC[lightNum];\n"
        "      float distanceVC = length(vertLightDirectionVC);\n"
        "      vertLightDirectionVC = normalize(vertLightDirectionVC);\n"
        "      attenuation = 1.0 /\n"
        "        (lightAttenuation[lightNum].x\n"
        "         + lightAttenuation[lightNum].y * distanceVC\n"
        "         + lightAttenuation[lightNum].z * distanceVC * distanceVC);\n"
        "      // per OpenGL standard cone angle is 90 or less for a spot light\n"
        "      if (lightConeAngle[lightNum] <= 90.0)\n"
        "        {\n"
        "        float coneDot = dot(vertLightDirectionVC, lightDirectionVC[lightNum]);\n"
        "        // if inside the cone\n"
        "        if (coneDot >= cos(radians(lightConeAngle[lightNum])))\n"
        "          {\n"
        "          attenuation = attenuation * pow(coneDot, lightExponent[lightNum]);\n"
        "          }\n"
        "        else\n"
        "          {\n"
        "          attenuation = 0.0;\n"
        "          }\n"
        "        }\n"
        "      }\n"
        "    float df = max(0.0, attenuation*dot(normalVCVSOutput, -vertLightDirectionVC));\n"
        "    diffuse += ((df" + shadowFactor + ") * lightColor[lightNum]);\n"
        "    if (dot(normalVCVSOutput, vertLightDirectionVC) < 0.0)\n"
        "      {\n"
        "      float sf = attenuation*pow( max(0.0, dot(lightHalfAngleVC[lightNum],normalVCVSOutput)), specularPower);\n"
        "      specular += ((sf" + shadowFactor + ") * lightColor[lightNum]);\n"
        "      }\n"
        "    }\n"
        "  diffuse = diffuse * diffuseColor;\n"
        "  specular = specular * specularColor;\n"
        "  gl_FragData[0] = vec4(ambientColor + diffuse + specular, opacity);\n"
        "  //VTK::Light::Impl",
        false
        );
      break;
  }

  // If rendering luminance values, write those values to the fragment
  if (info && info->Has(vtkLightingMapPass::RENDER_LUMINANCE()))
  {
    switch (this->LastLightComplexity[this->LastBoundBO])
    {
      case 0: // no lighting
        vtkShaderProgram::Substitute(FSSource, "//VTK::Light::Impl",
          "  gl_FragData[0] = vec4(0.0, 0.0, 0.0, 1.0);"
        );
        break;
      case 1: // headlight
      case 2: // light kit
      case 3: // positional
        vtkShaderProgram::Substitute(FSSource, "//VTK::Light::Impl",
          "  float ambientY = dot(vec3(0.2126, 0.7152, 0.0722), ambientColor);\n"
          "  gl_FragData[0] = vec4(ambientY, diffuse.x, specular.x, 1.0);"
        );
        break;
    }
  }

  shaders[vtkShader::Fragment]->SetSource(FSSource);
}

void vtkOpenGLPolyDataMapper::ReplaceShaderTCoord(
  std::map<vtkShader::Type, vtkShader *> shaders,
  vtkRenderer *, vtkActor *actor)
{
  if (this->DrawingEdges)
  {
    return;
  }

  if (!this->HaveTextures(actor))
  {
    return;
  }

  std::string VSSource = shaders[vtkShader::Vertex]->GetSource();
  std::string GSSource = shaders[vtkShader::Geometry]->GetSource();
  std::string FSSource = shaders[vtkShader::Fragment]->GetSource();

  // handle texture transformation matrix
  vtkInformation *info = actor->GetPropertyKeys();
  if (info && info->Has(vtkProp::GeneralTextureTransform()))
  {
    vtkShaderProgram::Substitute(VSSource, "//VTK::TCoord::Dec",
      "//VTK::TCoord::Dec\n"
      "uniform mat4 tcMatrix;",
      false);
    if (this->VBO->TCoordComponents == 1)
    {
      vtkShaderProgram::Substitute(VSSource, "//VTK::TCoord::Impl",
        "vec4 tcoordTmp = tcMatrix*vec4(tcoordMC,0.0,0.0,1.0);\n"
        "tcoordVCVSOutput = tcoordTmp.x/tcoordTmp.w;");
    }
    else
    {
      vtkShaderProgram::Substitute(VSSource, "//VTK::TCoord::Impl",
        "vec4 tcoordTmp = tcMatrix*vec4(tcoordMC,0.0,1.0);\n"
        "tcoordVCVSOutput = tcoordTmp.xy/tcoordTmp.w;");
    }
  }
  else
  {
    vtkShaderProgram::Substitute(VSSource, "//VTK::TCoord::Impl",
      "tcoordVCVSOutput = tcoordMC;");
  }

  // If 1 or 2 components per coordinates
  std::string tCoordType;
  std::string tCoordImpFSPre;
  std::string tCoordImpFSPost;
  if (this->VBO->TCoordComponents == 1)
  {
    tCoordType = "float";
    tCoordImpFSPre = "vec2(";
    tCoordImpFSPost = ", 0.0)";
  }
  else
  {
    tCoordType = "vec2";
    tCoordImpFSPre = "";
    tCoordImpFSPost = "";
  }

  std::string tCoordDecFS;
  std::string tCoordImpFS;
  std::vector<vtkTexture *> textures = this->GetTextures(actor);
  for (size_t i = 0; i < textures.size(); ++i)
  {
    vtkTexture *texture = textures[i];

    // Define texture
    std::stringstream ss;
    ss << "uniform sampler2D texture_" << i << ";\n";
    tCoordDecFS += ss.str();

    // Read texture color
    ss.str("");
    ss << "vec4 tcolor_" << i << " = texture2D(texture_" << i << ", "
       << tCoordImpFSPre << "tcoordVCVSOutput" << tCoordImpFSPost << "); // Read texture color\n";

    // Update color based on texture number of components
    int tNumComp = vtkOpenGLTexture::SafeDownCast(texture)->GetTextureObject()->GetComponents();
    switch (tNumComp)
    {
      case 1:
        ss << "tcolor_" << i << " = vec4(tcolor_" << i << ".r,tcolor_" << i << ".r,tcolor_" << i << ".r,1.0)";
        break;
      case 2:
        ss << "tcolor_" << i << " = vec4(tcolor_" << i << ".r,tcolor_" << i << ".r,tcolor_" << i << ".r,tcolor_" << i << ".g)";
        break;
      case 3:
        ss << "tcolor_" << i << " = vec4(tcolor_" << i << ".r,tcolor_" << i << ".g,tcolor_" << i << ".b,1.0)";
    }
    ss << "; // Update color based on texture nbr of components \n";

    // Define final color based on texture blending
    if(i == 0)
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
             << "tcolor.a = tcolor_" << i << ".a + tcolor.a * (1 - tcolor_" << i << " .a); // BLENDING: Replace\n\n";
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
          ss << "tcolor.rgb -= tcolor_" << i << ".rgb * tcolor_" << i << ".a; // BLENDING: Subtract\n\n";
          break;
        default:
          vtkDebugMacro(<< "No blending mode given, ignoring this texture colors.");
          ss << "// NO BLENDING MODE: ignoring this texture colors\n";
      }
    }
    tCoordImpFS += ss.str();
  }

  // Substitute in shader files
  vtkShaderProgram::Substitute(VSSource, "//VTK::TCoord::Dec",
    "attribute " + tCoordType + " tcoordMC;\n" +
    "varying " + tCoordType + " tcoordVCVSOutput;");
  vtkShaderProgram::Substitute(GSSource, "//VTK::TCoord::Dec",
    "in " + tCoordType + " tcoordVCVSOutput[];\n" +
    "out " + tCoordType + " tcoordVCGSOutput;");
  vtkShaderProgram::Substitute(GSSource, "//VTK::TCoord::Impl",
    "tcoordVCGSOutput = tcoordVCVSOutput[i];");
  vtkShaderProgram::Substitute(FSSource, "//VTK::TCoord::Dec",
    "varying " + tCoordType + " tcoordVCVSOutput;\n" + tCoordDecFS);

  // do texture mapping except for scalar coloring case which is
  // handled above
  if (!this->InterpolateScalarsBeforeMapping || !this->ColorCoordinates)
  {
    vtkShaderProgram::Substitute(FSSource, "//VTK::TCoord::Impl",
      tCoordImpFS + "gl_FragData[0] = clamp(gl_FragData[0],0.0,1.0) * tcolor;");
  }

  shaders[vtkShader::Vertex]->SetSource(VSSource);
  shaders[vtkShader::Geometry]->SetSource(GSSource);
  shaders[vtkShader::Fragment]->SetSource(FSSource);
}

void vtkOpenGLPolyDataMapper::ReplaceShaderPicking(
  std::map<vtkShader::Type, vtkShader *> shaders,
  vtkRenderer *, vtkActor *)
{
  std::string FSSource = shaders[vtkShader::Fragment]->GetSource();

  if (this->LastSelectionState >= vtkHardwareSelector::MIN_KNOWN_PASS)
  {
    if (this->HavePickScalars)
    {
      vtkShaderProgram::Substitute(FSSource,
        "//VTK::Picking::Dec",
        "uniform samplerBuffer textureC;");
      vtkShaderProgram::Substitute(FSSource, "//VTK::Picking::Impl",
        "  gl_FragData[0] = texelFetchBuffer(textureC, gl_PrimitiveID + PrimitiveIDOffset);\n"
        );
    }
    else
    {
      switch (this->LastSelectionState)
      {
        case vtkHardwareSelector::ID_LOW24:
          vtkShaderProgram::Substitute(FSSource,
          "//VTK::Picking::Impl",
          "  int idx = gl_PrimitiveID + 1 + PrimitiveIDOffset;\n"
          "  gl_FragData[0] = vec4(float(idx%256)/255.0, float((idx/256)%256)/255.0, float((idx/65536)%256)/255.0, 1.0);\n");
        break;
        case vtkHardwareSelector::ID_MID24:
          // this may yerk on openGL ES 2.0 so no really huge meshes in ES 2.0 OK
          vtkShaderProgram::Substitute(FSSource,
          "//VTK::Picking::Impl",
          "  int idx = (gl_PrimitiveID + 1 + PrimitiveIDOffset);\n idx = ((idx & 0xff000000) >> 24);\n"
          "  gl_FragData[0] = vec4(float(idx%256)/255.0, float((idx/256)%256)/255.0, float(idx/65536)/255.0, 1.0);\n");
        break;
        default:
          vtkShaderProgram::Substitute(FSSource, "//VTK::Picking::Dec",
            "uniform vec3 mapperIndex;");
          vtkShaderProgram::Substitute(FSSource,
            "//VTK::Picking::Impl",
            "  gl_FragData[0] = vec4(mapperIndex,1.0);\n");
      }
    }
  }
  shaders[vtkShader::Fragment]->SetSource(FSSource);
}

void vtkOpenGLPolyDataMapper::ReplaceShaderClip(
  std::map<vtkShader::Type, vtkShader *> shaders,
  vtkRenderer *, vtkActor *)
{
  std::string VSSource = shaders[vtkShader::Vertex]->GetSource();
  std::string FSSource = shaders[vtkShader::Fragment]->GetSource();

  if (this->GetNumberOfClippingPlanes())
  {
    // add all the clipping planes
    int numClipPlanes = this->GetNumberOfClippingPlanes();
    if (numClipPlanes > 6)
    {
      vtkErrorMacro(<< "OpenGL has a limit of 6 clipping planes");
      numClipPlanes = 6;
    }

    vtkShaderProgram::Substitute(VSSource, "//VTK::Clip::Dec",
      "uniform int numClipPlanes;\n"
      "uniform vec4 clipPlanes[6];\n"
      "varying float clipDistancesVSOutput[6];");
    vtkShaderProgram::Substitute(VSSource, "//VTK::Clip::Impl",
      "for (int planeNum = 0; planeNum < numClipPlanes; planeNum++)\n"
      "    {\n"
      "    clipDistancesVSOutput[planeNum] = dot(clipPlanes[planeNum], vertexMC);\n"
      "    }\n");
    vtkShaderProgram::Substitute(FSSource, "//VTK::Clip::Dec",
      "uniform int numClipPlanes;\n"
      "varying float clipDistancesVSOutput[6];");
    vtkShaderProgram::Substitute(FSSource, "//VTK::Clip::Impl",
      "for (int planeNum = 0; planeNum < numClipPlanes; planeNum++)\n"
      "    {\n"
      "    if (clipDistancesVSOutput[planeNum] < 0.0) discard;\n"
      "    }\n");
  }
  shaders[vtkShader::Vertex]->SetSource(VSSource);
  shaders[vtkShader::Fragment]->SetSource(FSSource);
}

void vtkOpenGLPolyDataMapper::ReplaceShaderNormal(
  std::map<vtkShader::Type, vtkShader *> shaders,
  vtkRenderer *, vtkActor *actor)
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
    vtkShaderProgram::Substitute(FSSource,
      "//VTK::Normal::Dec",
      "uniform float ZCalcS;\n"
      "uniform float ZCalcR;\n"
      "uniform int cameraParallel;\n"
      );
    vtkShaderProgram::Substitute(FSSource,
      "//VTK::Normal::Impl",

      " float xpos = 2.0*gl_PointCoord.x - 1.0;\n"
      " float ypos = 1.0 - 2.0*gl_PointCoord.y;\n"
      " float len2 = xpos*xpos+ ypos*ypos;\n"
      " if (len2 > 1.0) { discard; }\n"
      " vec3 normalVCVSOutput = normalize(\n"
      "   vec3(2.0*gl_PointCoord.x - 1.0, 1.0 - 2.0*gl_PointCoord.y, sqrt(1.0 - len2)));\n"

      " gl_FragDepth = gl_FragCoord.z + normalVCVSOutput.z*ZCalcS*ZCalcR;\n"
      " if (cameraParallel == 0) {\n"
      "  float ZCalcQ = (normalVCVSOutput.z*ZCalcR - 1.0);\n"
      "  gl_FragDepth = (ZCalcS - gl_FragCoord.z) / ZCalcQ + ZCalcS; }\n"
      );

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

    vtkShaderProgram::Substitute(FSSource,
      "//VTK::Normal::Dec",
      "varying vec3 tubeBasis1;\n"
      "varying vec3 tubeBasis2;\n"
      "uniform float ZCalcS;\n"
      "uniform float ZCalcR;\n"
      "uniform int cameraParallel;\n"
      );
    vtkShaderProgram::Substitute(FSSource,
      "//VTK::Normal::Impl",

      "float len2 = tubeBasis1.x*tubeBasis1.x + tubeBasis1.y*tubeBasis1.y;\n"
      "float lenZ = clamp(sqrt(1.0 - len2),0.0,1.0);\n"
      "vec3 normalVCVSOutput = normalize(tubeBasis1 + tubeBasis2*lenZ);\n"
      " gl_FragDepth = gl_FragCoord.z + lenZ*ZCalcS*ZCalcR/clamp(tubeBasis2.z,0.5,1.0);\n"
      " if (cameraParallel == 0) {\n"
      "  float ZCalcQ = (lenZ*ZCalcR/clamp(tubeBasis2.z,0.5,1.0) - 1.0);\n"
      "  gl_FragDepth = (ZCalcS - gl_FragCoord.z) / ZCalcQ + ZCalcS; }\n"
      );

    vtkShaderProgram::Substitute(GSSource,
      "//VTK::Normal::Dec",
      "in vec4 vertexVCVSOutput[];\n"
      "out vec3 tubeBasis1;\n"
      "out vec3 tubeBasis2;\n"
      );

    vtkShaderProgram::Substitute(GSSource,
      "//VTK::Normal::Start",
      "vec3 lineDir = normalize(vertexVCVSOutput[1].xyz - vertexVCVSOutput[0].xyz);\n"
      "tubeBasis2 = normalize(cross(lineDir, vec3(normal, 0.0)));\n"
      "tubeBasis2 = tubeBasis2*sign(tubeBasis2.z);\n"
      );

    vtkShaderProgram::Substitute(GSSource,
      "//VTK::Normal::Impl",
      "tubeBasis1 = 2.0*vec3(normal*((j+1)%2 - 0.5), 0.0);\n"
      );

    shaders[vtkShader::Geometry]->SetSource(GSSource);
    shaders[vtkShader::Fragment]->SetSource(FSSource);
    return;
  }

  if (this->LastLightComplexity[this->LastBoundBO] > 0)
  {
    std::string VSSource = shaders[vtkShader::Vertex]->GetSource();
    std::string GSSource = shaders[vtkShader::Geometry]->GetSource();

    // if we have point normals provided
    if (this->VBO->NormalOffset)
    {
      vtkShaderProgram::Substitute(VSSource,
        "//VTK::Normal::Dec",
        "attribute vec3 normalMC;\n"
        "uniform mat3 normalMatrix;\n"
        "varying vec3 normalVCVSOutput;");
      vtkShaderProgram::Substitute(VSSource,
        "//VTK::Normal::Impl",
        "normalVCVSOutput = normalMatrix * normalMC;");
      vtkShaderProgram::Substitute(GSSource,
        "//VTK::Normal::Dec",
        "in vec3 normalVCVSOutput[];\n"
        "out vec3 normalVCGSOutput;");
      vtkShaderProgram::Substitute(GSSource,
        "//VTK::Normal::Impl",
        "normalVCGSOutput = normalVCVSOutput[i];");
      vtkShaderProgram::Substitute(FSSource,
        "//VTK::Normal::Dec",
        "varying vec3 normalVCVSOutput;");
      vtkShaderProgram::Substitute(FSSource,
        "//VTK::Normal::Impl",
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
      vtkShaderProgram::Substitute(FSSource,
        "//VTK::Normal::Dec",
        "uniform mat3 normalMatrix;\n"
        "uniform samplerBuffer textureN;\n");
      if (this->CellNormalTexture->GetVTKDataType() == VTK_FLOAT)
      {
        vtkShaderProgram::Substitute(FSSource,
          "//VTK::Normal::Impl",
          "vec3 normalVCVSOutput = \n"
          "    texelFetchBuffer(textureN, gl_PrimitiveID + PrimitiveIDOffset).xyz;\n"
          "normalVCVSOutput = normalize(normalMatrix * normalVCVSOutput);\n"
          "  if (gl_FrontFacing == false) { normalVCVSOutput = -normalVCVSOutput; }\n"
          );
      }
      else
      {
        vtkShaderProgram::Substitute(FSSource,
            "//VTK::Normal::Impl",
            "vec3 normalVCVSOutput = \n"
            "    texelFetchBuffer(textureN, gl_PrimitiveID + PrimitiveIDOffset).xyz;\n"
            "normalVCVSOutput = normalVCVSOutput * 255.0/127.0 - 1.0;\n"
            "normalVCVSOutput = normalize(normalMatrix * normalVCVSOutput);\n"
            "  if (gl_FrontFacing == false) { normalVCVSOutput = -normalVCVSOutput; }\n"
            );
        shaders[vtkShader::Fragment]->SetSource(FSSource);
        return;
      }
    }

    // OK we have no point or cell normals, so compute something
    // we have a forumla for wireframe
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
      vtkShaderProgram::Substitute(
            FSSource,"//VTK::UniformFlow::Impl",
            "  vec3 fdx = vec3(dFdx(vertexVC.x),dFdx(vertexVC.y),dFdx(vertexVC.z));\n"
            "  vec3 fdy = vec3(dFdy(vertexVC.x),dFdy(vertexVC.y),dFdy(vertexVC.z));\n"
            "  //VTK::UniformFlow::Impl\n" // For further replacements
            );
      vtkShaderProgram::Substitute(FSSource,"//VTK::Normal::Impl",
        "vec3 normalVCVSOutput;\n"
        "  fdx = normalize(fdx);\n"
        "  fdy = normalize(fdy);\n"
        "  if (abs(fdx.x) > 0.0)\n"
        "    { normalVCVSOutput = normalize(cross(vec3(fdx.y, -fdx.x, 0.0), fdx)); }\n"
        "  else { normalVCVSOutput = normalize(cross(vec3(fdy.y, -fdy.x, 0.0), fdy));}"
        );
    }
    else // not lines, so surface
    {
      vtkShaderProgram::Substitute(FSSource,
        "//VTK::Normal::Dec",
        "uniform int cameraParallel;");

      vtkShaderProgram::Substitute(
            FSSource,"//VTK::UniformFlow::Impl",
            "vec3 fdx = vec3(dFdx(vertexVC.x),dFdx(vertexVC.y),dFdx(vertexVC.z));\n"
            "  vec3 fdy = vec3(dFdy(vertexVC.x),dFdy(vertexVC.y),dFdy(vertexVC.z));\n"
            "  //VTK::UniformFlow::Impl\n" // For further replacements
            );
      vtkShaderProgram::Substitute(FSSource,"//VTK::Normal::Impl",
        "  fdx = normalize(fdx);\n"
        "  fdy = normalize(fdy);\n"
        "  vec3 normalVCVSOutput = normalize(cross(fdx,fdy));\n"
        // the code below is faster, but does not work on some devices
        //"vec3 normalVC = normalize(cross(dFdx(vertexVC.xyz), dFdy(vertexVC.xyz)));\n"
        "  if (cameraParallel == 1 && normalVCVSOutput.z < 0.0) { normalVCVSOutput = -1.0*normalVCVSOutput; }\n"
        "  if (cameraParallel == 0 && dot(normalVCVSOutput,vertexVC.xyz) > 0.0) { normalVCVSOutput = -1.0*normalVCVSOutput; }"
        );
    }
    shaders[vtkShader::Fragment]->SetSource(FSSource);
  }
}

void vtkOpenGLPolyDataMapper::ReplaceShaderPositionVC(
  std::map<vtkShader::Type, vtkShader *> shaders,
  vtkRenderer *, vtkActor *)
{
  std::string VSSource = shaders[vtkShader::Vertex]->GetSource();
  std::string GSSource = shaders[vtkShader::Geometry]->GetSource();
  std::string FSSource = shaders[vtkShader::Fragment]->GetSource();

 // do we need the vertex in the shader in View Coordinates
  if (this->LastLightComplexity[this->LastBoundBO] > 0)
  {
    vtkShaderProgram::Substitute(VSSource,
      "//VTK::PositionVC::Dec",
      "varying vec4 vertexVCVSOutput;");
    vtkShaderProgram::Substitute(VSSource,
      "//VTK::PositionVC::Impl",
      "vertexVCVSOutput = MCVCMatrix * vertexMC;\n"
      "  gl_Position = MCDCMatrix * vertexMC;\n");
    vtkShaderProgram::Substitute(VSSource,
      "//VTK::Camera::Dec",
      "uniform mat4 MCDCMatrix;\n"
      "uniform mat4 MCVCMatrix;");
    vtkShaderProgram::Substitute(GSSource,
      "//VTK::PositionVC::Dec",
      "in vec4 vertexVCVSOutput[];\n"
      "out vec4 vertexVCGSOutput;");
    vtkShaderProgram::Substitute(GSSource,
      "//VTK::PositionVC::Impl",
      "vertexVCGSOutput = vertexVCVSOutput[i];");
    vtkShaderProgram::Substitute(FSSource,
      "//VTK::PositionVC::Dec",
      "varying vec4 vertexVCVSOutput;");
    vtkShaderProgram::Substitute(FSSource,
      "//VTK::PositionVC::Impl",
      "vec4 vertexVC = vertexVCVSOutput;");
  }
  else
  {
    vtkShaderProgram::Substitute(VSSource,
      "//VTK::Camera::Dec",
      "uniform mat4 MCDCMatrix;");
    vtkShaderProgram::Substitute(VSSource,
      "//VTK::PositionVC::Impl",
      "  gl_Position = MCDCMatrix * vertexMC;\n");
  }
  shaders[vtkShader::Vertex]->SetSource(VSSource);
  shaders[vtkShader::Geometry]->SetSource(GSSource);
  shaders[vtkShader::Fragment]->SetSource(FSSource);
}

void vtkOpenGLPolyDataMapper::ReplaceShaderPrimID(
  std::map<vtkShader::Type, vtkShader *> shaders,
  vtkRenderer *, vtkActor *)
{
  std::string VSSource = shaders[vtkShader::Vertex]->GetSource();
  std::string GSSource = shaders[vtkShader::Geometry]->GetSource();
  std::string FSSource = shaders[vtkShader::Fragment]->GetSource();

  // are we handling the apple bug?
  if (this->AppleBugPrimIDs.size())
  {
    vtkShaderProgram::Substitute(VSSource,"//VTK::PrimID::Dec",
      "attribute vec4 appleBugPrimID;\n"
      "varying vec4 applePrimIDVSOutput;");
    vtkShaderProgram::Substitute(VSSource,"//VTK::PrimID::Impl",
      "applePrimIDVSOutput = appleBugPrimID;");
    vtkShaderProgram::Substitute(GSSource,
      "//VTK::PrimID::Dec",
      "in  vec4 applePrimIDVSOutput[];\n"
      "out vec4 applePrimIDGSOutput;");
    vtkShaderProgram::Substitute(GSSource,
      "//VTK::PrimID::Impl",
      "applePrimIDGSOutput = applePrimIDVSOutput[i];");
    vtkShaderProgram::Substitute(FSSource,"//VTK::PrimID::Dec",
      "varying vec4 applePrimIDVSOutput;");
     vtkShaderProgram::Substitute(FSSource,"//VTK::PrimID::Impl",
       "int vtkPrimID = int(applePrimIDVSOutput[0]*255.1) + int(applePrimIDVSOutput[1]*255.1)*256 + int(applePrimIDVSOutput[2]*255.1)*65536;");
    vtkShaderProgram::Substitute(FSSource,"gl_PrimitiveID","vtkPrimID");
  }
  else
  {
    if (this->HaveCellNormals || this->HaveCellScalars || this->HavePickScalars)
    {
      vtkShaderProgram::Substitute(GSSource,
        "//VTK::PrimID::Impl",
        "gl_PrimitiveID = gl_PrimitiveIDIn;");
    }
  }
  shaders[vtkShader::Vertex]->SetSource(VSSource);
  shaders[vtkShader::Geometry]->SetSource(GSSource);
  shaders[vtkShader::Fragment]->SetSource(FSSource);
}

void vtkOpenGLPolyDataMapper::ReplaceShaderCoincidentOffset(
  std::map<vtkShader::Type, vtkShader *> shaders,
  vtkRenderer *ren, vtkActor *actor)
{
  float factor = 0.0;
  float offset = 0.0;
  this->GetCoincidentParameters(ren, actor,factor,offset);

  // if we need an offset handle it here
  // The value of .000016 is suitable for depth buffers
  // of at least 16 bit depth. We do not query the depth
  // right now because we would need some mechanism to
  // cache the result taking into account FBO changes etc.
  if (factor != 0.0 || offset != 0.0)
  {
    std::string FSSource = shaders[vtkShader::Fragment]->GetSource();

    vtkShaderProgram::Substitute(FSSource,
      "//VTK::Coincident::Dec",
      "uniform float cfactor;\n"
      "uniform float coffset;");
    if (factor != 0.0)
    {
      vtkShaderProgram::Substitute(FSSource,
        "//VTK::UniformFlow::Impl",
        "float cscale = length(vec2(dFdx(gl_FragCoord.z),dFdy(gl_FragCoord.z)));\n"
        "  //VTK::UniformFlow::Impl\n" // for other replacements
        );
      vtkShaderProgram::Substitute(FSSource, "//VTK::Depth::Impl",
        "gl_FragDepth = gl_FragCoord.z + cfactor*cscale + 0.000016*coffset;\n");
    }
    else
    {
      vtkShaderProgram::Substitute(FSSource,
        "//VTK::Depth::Impl",
        "gl_FragDepth = gl_FragCoord.z + 0.000016*coffset;\n"
        );
    }
    shaders[vtkShader::Fragment]->SetSource(FSSource);
  }
}

void vtkOpenGLPolyDataMapper::ReplaceShaderDepth(
    std::map<vtkShader::Type, vtkShader *> shaders,
    vtkRenderer *, vtkActor *)
{
  // If MSAA is enabled, don't write to gl_FragDepth unless we absolutely have
  // to. See VTK issue 16899.
#if GL_ES_VERSION_3_0 != 1
  bool multisampling = glIsEnabled(GL_MULTISAMPLE) == GL_TRUE;
#else
  bool multisample = false;
#endif

  if (!multisampling)
  {
    std::string FSSource = shaders[vtkShader::Fragment]->GetSource();
    vtkShaderProgram::Substitute(FSSource,
                                 "//VTK::Depth::Impl",
                                 "gl_FragDepth = gl_FragCoord.z;");
    shaders[vtkShader::Fragment]->SetSource(FSSource);
  }
}

void vtkOpenGLPolyDataMapper::ReplaceShaderValues(
  std::map<vtkShader::Type, vtkShader *> shaders,
  vtkRenderer *ren, vtkActor *actor)
{
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
  this->ReplaceShaderRenderPass(shaders, ren, actor);

  //cout << "VS: " << shaders[vtkShader::Vertex]->GetSource() << endl;
  //cout << "GS: " << shaders[vtkShader::Geometry]->GetSource() << endl;
  //cout << "FS: " << shaders[vtkShader::Fragment]->GetSource() << endl;
}

bool vtkOpenGLPolyDataMapper::DrawingSpheres(vtkOpenGLHelper &cellBO, vtkActor *actor)
{
  return ((&cellBO == &this->Points ||
      actor->GetProperty()->GetRepresentation() == VTK_POINTS) &&
      actor->GetProperty()->GetRenderPointsAsSpheres() &&
      !this->DrawingEdges);
}

bool vtkOpenGLPolyDataMapper::DrawingTubes(vtkOpenGLHelper &cellBO, vtkActor *actor)
{
  return (actor->GetProperty()->GetRenderLinesAsTubes() &&
      (&cellBO == &this->Lines ||
       &cellBO == &this->TrisEdges ||
       &cellBO == &this->TriStripsEdges ||
       (actor->GetProperty()->GetRepresentation() == VTK_WIREFRAME &&
        &cellBO != &this->Points)));
}

//-----------------------------------------------------------------------------
bool vtkOpenGLPolyDataMapper::GetNeedToRebuildShaders(
  vtkOpenGLHelper &cellBO, vtkRenderer* ren, vtkActor *actor)
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

  // we sphering or tubing? Yes I made sphere into a verb
  if (this->DrawingTubes(cellBO, actor) || this->DrawingSpheres(cellBO, actor))
  {
    needLighting = true;
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

  if (this->LastLightComplexity[&cellBO] != lightComplexity)
  {
    this->LightComplexityChanged[&cellBO].Modified();
    this->LastLightComplexity[&cellBO] = lightComplexity;
  }

  // Have the renderpasses changed?
  vtkMTimeType renderPassMTime = this->GetRenderPassStageMTime(actor);

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
      cellBO.ShaderSourceTime < renderPassMTime ||
      cellBO.ShaderSourceTime < this->LightComplexityChanged[&cellBO]
#if GL_ES_VERSION_2_0 != 1 && GL_ES_VERSION_3_0 != 1
      || this->ValuePassHelper->RequiresShaderRebuild()
#endif
      )
  {
    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::UpdateShaders(
  vtkOpenGLHelper &cellBO, vtkRenderer* ren, vtkActor *actor)
{
  vtkOpenGLRenderWindow *renWin = vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());

  cellBO.VAO->Bind();
  this->LastBoundBO = &cellBO;

  // has something changed that would require us to recreate the shader?
  if (this->GetNeedToRebuildShaders(cellBO, ren, actor))
  {
    // build the shader source code
    std::map<vtkShader::Type,vtkShader *> shaders;
    vtkShader *vss = vtkShader::New();
    vss->SetType(vtkShader::Vertex);
    shaders[vtkShader::Vertex] = vss;
    vtkShader *gss = vtkShader::New();
    gss->SetType(vtkShader::Geometry);
    shaders[vtkShader::Geometry] = gss;
    vtkShader *fss = vtkShader::New();
    fss->SetType(vtkShader::Fragment);
    shaders[vtkShader::Fragment] = fss;

    this->BuildShaders(shaders, ren, actor);

    // compile and bind the program if needed
    vtkShaderProgram *newShader =
      renWin->GetShaderCache()->ReadyShaderProgram(shaders);

    vss->Delete();
    fss->Delete();
    gss->Delete();

    // if the shader changed reinitialize the VAO
    if (newShader != cellBO.Program)
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
  }

  this->SetMapperShaderParameters(cellBO, ren, actor);
  this->SetPropertyShaderParameters(cellBO, ren, actor);
  this->SetCameraShaderParameters(cellBO, ren, actor);
  this->SetLightingShaderParameters(cellBO, ren, actor);

  // allow the program to set what it wants
  this->InvokeEvent(vtkCommand::UpdateShaderEvent,&cellBO);

  vtkOpenGLCheckErrorMacro("failed after UpdateShader");
}

void vtkOpenGLPolyDataMapper::SetMapperShaderParameters(vtkOpenGLHelper &cellBO,
                                                      vtkRenderer* ren, vtkActor *actor)
{
  // Now to update the VAO too, if necessary.
  cellBO.Program->SetUniformi("PrimitiveIDOffset",
    this->PrimitiveIDOffset);

  if (cellBO.IBO->IndexCount && (this->VBOBuildTime > cellBO.AttributeUpdateTime ||
      cellBO.ShaderSourceTime > cellBO.AttributeUpdateTime))
  {
    cellBO.VAO->Bind();
    if (cellBO.Program->IsAttributeUsed("vertexMC"))
    {
      if (!cellBO.VAO->AddAttributeArray(cellBO.Program, this->VBO,
                                         "vertexMC", this->VBO->VertexOffset,
                                         this->VBO->Stride, VTK_FLOAT, 3,
                                         false))
      {
        vtkErrorMacro(<< "Error setting 'vertexMC' in shader VAO.");
      }
    }
    if (this->VBO->NormalOffset && this->LastLightComplexity[&cellBO] > 0 &&
        cellBO.Program->IsAttributeUsed("normalMC"))
    {
      if (!cellBO.VAO->AddAttributeArray(cellBO.Program, this->VBO,
                                      "normalMC", this->VBO->NormalOffset,
                                      this->VBO->Stride, VTK_FLOAT, 3, false))
      {
        vtkErrorMacro(<< "Error setting 'normalMC' in shader VAO.");
      }
    }
    if (this->VBO->TCoordComponents && !this->DrawingEdges &&
        cellBO.Program->IsAttributeUsed("tcoordMC"))
    {
      if (!cellBO.VAO->AddAttributeArray(cellBO.Program, this->VBO,
                                      "tcoordMC", this->VBO->TCoordOffset,
                                      this->VBO->Stride, VTK_FLOAT, this->VBO->TCoordComponents, false))
      {
        vtkErrorMacro(<< "Error setting 'tcoordMC' in shader VAO.");
      }
    }
    if (this->VBO->ColorComponents != 0 && !this->DrawingEdges &&
        cellBO.Program->IsAttributeUsed("scalarColor"))
    {
      if (!cellBO.VAO->AddAttributeArray(cellBO.Program, this->VBO,
                                      "scalarColor", this->VBO->ColorOffset,
                                      this->VBO->Stride, VTK_UNSIGNED_CHAR,
                                      this->VBO->ColorComponents, true))
      {
        vtkErrorMacro(<< "Error setting 'scalarColor' in shader VAO.");
      }
    }
    if (this->AppleBugPrimIDs.size() &&
        cellBO.Program->IsAttributeUsed("appleBugPrimID"))
    {
      if (!cellBO.VAO->AddAttributeArray(cellBO.Program,
          this->AppleBugPrimIDBuffer,
          "appleBugPrimID",
           0, sizeof(float), VTK_UNSIGNED_CHAR, 4, true))
      {
        vtkErrorMacro(<< "Error setting 'appleBugPrimID' in shader VAO.");
      }
    }

#if GL_ES_VERSION_2_0 != 1 && GL_ES_VERSION_3_0 != 1
    if (this->ValuePassHelper->GetRenderingMode() == vtkValuePass::FLOATING_POINT)
    {
      this->ValuePassHelper->BindAttributes(cellBO);
    }
#endif

    cellBO.AttributeUpdateTime.Modified();
  }

  if (this->HaveTextures(actor))
  {
    std::vector<vtkTexture *> textures = this->GetTextures(actor);
    for (size_t i = 0; i < textures.size(); ++i)
    {
      vtkTexture *texture = textures[i];
      std::stringstream ss; ss << "texture_" << i;
      std::string s = ss.str();
      if (texture && cellBO.Program->IsUniformUsed(s.c_str()))
      {
        int tunit = vtkOpenGLTexture::SafeDownCast(texture)->GetTextureUnit();
        cellBO.Program->SetUniformi(s.c_str(), tunit);
      }
    }

    // check for tcoord transform matrix
    vtkInformation *info = actor->GetPropertyKeys();
    vtkOpenGLCheckErrorMacro("failed after Render");
    if (info && info->Has(vtkProp::GeneralTextureTransform()) &&
        cellBO.Program->IsUniformUsed("tcMatrix"))
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

  if ((this->HaveCellScalars || this->HavePickScalars) &&
      cellBO.Program->IsUniformUsed("textureC"))
  {
    int tunit = this->CellScalarTexture->GetTextureUnit();
    cellBO.Program->SetUniformi("textureC", tunit);
  }

  if (this->HaveCellNormals && cellBO.Program->IsUniformUsed("textureN"))
  {
    int tunit = this->CellNormalTexture->GetTextureUnit();
    cellBO.Program->SetUniformi("textureN", tunit);
  }

#if GL_ES_VERSION_2_0 != 1 && GL_ES_VERSION_3_0 != 1
  if (this->ValuePassHelper->GetRenderingMode() == vtkValuePass::FLOATING_POINT)
  {
    this->ValuePassHelper->BindUniforms(cellBO);
  }
#endif

  // Handle render pass setup:
  vtkInformation *info = actor->GetPropertyKeys();
  if (info && info->Has(vtkOpenGLRenderPass::RenderPasses()))
  {
    int numRenderPasses = info->Length(vtkOpenGLRenderPass::RenderPasses());
    for (int i = 0; i < numRenderPasses; ++i)
    {
      vtkObjectBase *rpBase = info->Get(vtkOpenGLRenderPass::RenderPasses(), i);
      vtkOpenGLRenderPass *rp = static_cast<vtkOpenGLRenderPass*>(rpBase);
      if (!rp->SetShaderParameters(cellBO.Program, this, actor))
      {
        vtkErrorMacro("RenderPass::SetShaderParameters failed for renderpass: "
                      << rp->GetClassName());
      }
    }
  }

  vtkHardwareSelector* selector = ren->GetSelector();
  bool picking = (ren->GetRenderWindow()->GetIsPicking() || selector != NULL);
  if (picking && cellBO.Program->IsUniformUsed("mapperIndex"))
  {
    if (selector)
    {
      if (selector->GetCurrentPass() < vtkHardwareSelector::ID_LOW24)
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

  if (this->GetNumberOfClippingPlanes() &&
      cellBO.Program->IsUniformUsed("numClipPlanes") &&
      cellBO.Program->IsUniformUsed("clipPlanes"))
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

  // handle wide lines
  if (this->HaveWideLines(ren, actor) &&
      cellBO.Program->IsUniformUsed("lineWidthNVC"))
  {
      int vp[4];
      glGetIntegerv(GL_VIEWPORT, vp);
      float lineWidth[2];
      lineWidth[0] = 2.0*actor->GetProperty()->GetLineWidth()/vp[2];
      lineWidth[1] = 2.0*actor->GetProperty()->GetLineWidth()/vp[3];
      cellBO.Program->SetUniform2f("lineWidthNVC",lineWidth);
  }

  // handle coincident
  if (cellBO.Program->IsUniformUsed("coffset"))
  {
    float factor, offset;
    this->GetCoincidentParameters(ren, actor,factor,offset);
    cellBO.Program->SetUniformf("coffset",offset);
    // cfactor isn't always used when coffset is.
    if (cellBO.Program->IsUniformUsed("cfactor"))
    {
      cellBO.Program->SetUniformf("cfactor", factor);
    }
  }
}

//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::SetLightingShaderParameters(
  vtkOpenGLHelper &cellBO,
  vtkRenderer* ren,
  vtkActor *actor)
{
  // for unlit and headlight there are no lighting parameters
  if (this->LastLightComplexity[&cellBO] < 2 ||
      (this->DrawingEdges && !this->DrawingTubes(cellBO, actor)))
  {
    return;
  }

  vtkShaderProgram *program = cellBO.Program;

  // check for shadow maps
  vtkInformation *info = actor->GetPropertyKeys();
  if (info && info->Has(vtkShadowMapPass::ShadowMapPass()))
  {
    vtkShadowMapPass *smp = vtkShadowMapPass::SafeDownCast(
      info->Get(vtkShadowMapPass::ShadowMapPass()));
    if (smp)
    {
      smp->SetUniforms(program);
    }
  }

  // for lightkit case there are some parameters to set
  vtkCamera *cam = ren->GetActiveCamera();
  vtkTransform* viewTF = cam->GetModelViewTransformObject();

  // bind some light settings
  int numberOfLights = 0;
  vtkLightCollection *lc = ren->GetLights();
  vtkLight *light;

  bool renderLuminance = info &&
    info->Has(vtkLightingMapPass::RENDER_LUMINANCE());

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
      if (renderLuminance)
      {
        lightColor[numberOfLights][0] = intensity;
        lightColor[numberOfLights][1] = intensity;
        lightColor[numberOfLights][2] = intensity;
      }
      else
      {
        lightColor[numberOfLights][0] = dColor[0] * intensity;
        lightColor[numberOfLights][1] = dColor[1] * intensity;
        lightColor[numberOfLights][2] = dColor[2] * intensity;
      }
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
  if (this->LastLightComplexity[&cellBO] < 3)
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
void vtkOpenGLPolyDataMapper::SetCameraShaderParameters(vtkOpenGLHelper &cellBO,
                                                    vtkRenderer* ren, vtkActor *actor)
{
  vtkShaderProgram *program = cellBO.Program;

  vtkOpenGLCamera *cam = (vtkOpenGLCamera *)(ren->GetActiveCamera());

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
      program->SetUniformf("ZCalcS", vcdc->GetElement(2,2));
    }
    else
    {
      program->SetUniformf("ZCalcS", -0.5*vcdc->GetElement(2,2) + 0.5);
    }
    if (this->DrawingSpheres(cellBO, actor))
    {
      program->SetUniformf("ZCalcR",
        actor->GetProperty()->GetPointSize()/
          (ren->GetSize()[0] * vcdc->GetElement(0,0)));
    }
    else
    {
      program->SetUniformf("ZCalcR",
        actor->GetProperty()->GetLineWidth()/
          (ren->GetSize()[0] * vcdc->GetElement(0,0)));
    }
  }

  if (this->VBO->GetCoordShiftAndScaleEnabled())
  {
    if (!actor->GetIsIdentity())
    {
      vtkMatrix4x4* mcwc;
      vtkMatrix3x3* anorms;
      ((vtkOpenGLActor*)actor)->GetKeyMatrices(mcwc,anorms);
      vtkMatrix4x4::Multiply4x4(this->VBOShiftScale.GetPointer(), mcwc, this->TempMatrix4);
      vtkMatrix4x4::Multiply4x4(this->TempMatrix4, wcdc, this->TempMatrix4);
      program->SetUniformMatrix("MCDCMatrix", this->TempMatrix4);
      if (program->IsUniformUsed("MCVCMatrix"))
      {
        vtkMatrix4x4::Multiply4x4(this->VBOShiftScale.GetPointer(), mcwc, this->TempMatrix4);
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
      vtkMatrix4x4::Multiply4x4(this->VBOShiftScale.GetPointer(), wcdc, this->TempMatrix4);
      program->SetUniformMatrix("MCDCMatrix", this->TempMatrix4);
      if (program->IsUniformUsed("MCVCMatrix"))
      {
        vtkMatrix4x4::Multiply4x4(this->VBOShiftScale.GetPointer(), wcvc, this->TempMatrix4);
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
      vtkMatrix4x4 *mcwc;
      vtkMatrix3x3 *anorms;
      ((vtkOpenGLActor *)actor)->GetKeyMatrices(mcwc,anorms);
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
void vtkOpenGLPolyDataMapper::SetPropertyShaderParameters(vtkOpenGLHelper &cellBO,
                                                       vtkRenderer*, vtkActor *actor)
{
  vtkShaderProgram *program = cellBO.Program;

  vtkProperty *ppty = actor->GetProperty();

  {
  // Query the property for some of the properties that can be applied.
  float opacity = static_cast<float>(ppty->GetOpacity());
  double *aColor = this->DrawingEdges ?
    ppty->GetEdgeColor() : ppty->GetAmbientColor();
  double aIntensity = (this->DrawingEdges && !this->DrawingTubes(cellBO, actor))
    ? 1.0 : ppty->GetAmbient();
  float ambientColor[3] = {static_cast<float>(aColor[0] * aIntensity),
    static_cast<float>(aColor[1] * aIntensity),
    static_cast<float>(aColor[2] * aIntensity)};

  double *dColor = this->DrawingEdges ?
    ppty->GetEdgeColor() : ppty->GetDiffuseColor();
  double dIntensity = (this->DrawingEdges && !this->DrawingTubes(cellBO, actor))
    ? 0.0 : ppty->GetDiffuse();
  float diffuseColor[3] = {static_cast<float>(dColor[0] * dIntensity),
    static_cast<float>(dColor[1] * dIntensity),
    static_cast<float>(dColor[2] * dIntensity)};

  double *sColor = ppty->GetSpecularColor();
  double sIntensity = (this->DrawingEdges && !this->DrawingTubes(cellBO, actor))
    ? 0.0 : ppty->GetSpecular();
  float specularColor[3] = {static_cast<float>(sColor[0] * sIntensity),
    static_cast<float>(sColor[1] * sIntensity),
    static_cast<float>(sColor[2] * sIntensity)};
  double specularPower = ppty->GetSpecularPower();

  program->SetUniformf("opacityUniform", opacity);
  program->SetUniform3f("ambientColorUniform", ambientColor);
  program->SetUniform3f("diffuseColorUniform", diffuseColor);
  // we are done unless we have lighting
  if (this->LastLightComplexity[&cellBO] < 1)
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
    if (this->LastLightComplexity[&cellBO] < 1)
    {
      return;
    }
    program->SetUniform3f("specularColorUniformBF", specularColor);
    program->SetUniformf("specularPowerUniformBF", specularPower);
  }

}

namespace
{
// helper to get the state of picking
int getPickState(vtkRenderer *ren)
{
  vtkHardwareSelector* selector = ren->GetSelector();
  if (selector)
  {
    return selector->GetCurrentPass();
  }

  if (ren->GetRenderWindow()->GetIsPicking())
  {
    return vtkHardwareSelector::ACTOR_PASS;
  }

  return vtkHardwareSelector::MIN_KNOWN_PASS - 1;
}
}

void vtkOpenGLPolyDataMapper::GetCoincidentParameters(
  vtkRenderer* ren, vtkActor *actor,
  float &factor, float &offset)
{
  // 1. ResolveCoincidentTopology is On and non zero for this primitive
  // type
  factor = 0.0;
  offset = 0.0;
  if ( this->GetResolveCoincidentTopology() == VTK_RESOLVE_SHIFT_ZBUFFER )
  {
    // do something rough is better than nothing
    double zRes = this->GetResolveCoincidentTopologyZShift(); // 0 is no shift 1 is big shift
    double f = zRes*4.0;
    factor = f;
  }

  vtkProperty *prop = actor->GetProperty();
  if ((this->GetResolveCoincidentTopology() == VTK_RESOLVE_POLYGON_OFFSET) ||
      (prop->GetEdgeVisibility() && prop->GetRepresentation() == VTK_SURFACE))
  {
    double f = 0.0;
    double u = 0.0;
    if (this->LastBoundBO == &this->Points ||
        prop->GetRepresentation() == VTK_POINTS)
    {
      this->GetCoincidentTopologyPointOffsetParameter(u);
    }
    else if (this->LastBoundBO == &this->Lines ||
        prop->GetRepresentation() == VTK_WIREFRAME)
    {
      this->GetCoincidentTopologyLineOffsetParameters(f,u);
    }
    else if (this->LastBoundBO == &this->Tris ||
          this->LastBoundBO == &this->TriStrips)
    {
      this->GetCoincidentTopologyPolygonOffsetParameters(f,u);
    }
    if (this->LastBoundBO == &this->TrisEdges ||
        this->LastBoundBO == &this->TriStripsEdges)
    {
      this->GetCoincidentTopologyPolygonOffsetParameters(f,u);
      f /= 2;
      u /= 2;
    }
    factor = f;
    offset = u;
  }

  // hardware picking always offset due to saved zbuffer
  // This gets you above the saved surface depth buffer.
  vtkHardwareSelector* selector = ren->GetSelector();
  if (selector &&
      selector->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    offset -= 2.0;
    return;
  }
}

//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::RenderPieceStart(vtkRenderer* ren, vtkActor *actor)
{
  // Set the PointSize and LineWidget
#if GL_ES_VERSION_2_0 != 1
  glPointSize(actor->GetProperty()->GetPointSize()); // not on ES2
#endif

  this->TimeToDraw = 0.0;

#if GL_ES_VERSION_2_0 != 1 && GL_ES_VERSION_3_0 != 1
  if (this->TimerQuery == 0)
  {
    glGenQueries(1, static_cast<GLuint*>(&this->TimerQuery));
  }
  else
  {
    GLint timerAvailable = 0;
    glGetQueryObjectiv(static_cast<GLuint>(this->TimerQuery),
      GL_QUERY_RESULT_AVAILABLE, &timerAvailable);

    if (timerAvailable)
    {
      // See how much time the rendering of the mapper took
      // in nanoseconds during the previous frame
      GLuint timeElapsed = 0;
      glGetQueryObjectuiv(static_cast<GLuint>(this->TimerQuery),
        GL_QUERY_RESULT, &timeElapsed);
      // Set the rendering time for this frame with the previous one
      this->TimeToDraw = timeElapsed / 1.0e9;
    }
  }

  glBeginQuery(GL_TIME_ELAPSED, static_cast<GLuint>(this->TimerQuery));
#endif

  vtkHardwareSelector* selector = ren->GetSelector();
  int picking = getPickState(ren);
  if (this->LastSelectionState != picking)
  {
    this->SelectionStateChanged.Modified();
    this->LastSelectionState = picking;
  }

  // render points for point picking in a special way
  if (selector &&
      selector->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    glDepthMask(GL_FALSE);
  }
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

  this->PrimitiveIDOffset = 0;

  // make sure the BOs are up to date
  this->UpdateBufferObjects(ren, actor);

  if (this->HaveCellScalars || this->HavePickScalars)
  {
    this->CellScalarTexture->Activate();
  }
  if (this->HaveCellNormals)
  {
    this->CellNormalTexture->Activate();
  }

#if GL_ES_VERSION_2_0 != 1 && GL_ES_VERSION_3_0 != 1
  if (this->ValuePassHelper->GetRenderingMode() == vtkValuePass::FLOATING_POINT)
  {
    this->ValuePassHelper->RenderPieceStart(actor, this->CurrentInput);
  }
#endif

  // If we are coloring by texture, then load the texture map.
  // Use Map as indicator, because texture hangs around.
  if (this->ColorTextureMap)
  {
    this->InternalColorTexture->Load(ren);
  }

  // Bind the OpenGL, this is shared between the different primitive/cell types.
  this->VBO->Bind();
  this->LastBoundBO = NULL;
}

//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::RenderPieceDraw(vtkRenderer* ren, vtkActor *actor)
{
  int representation = actor->GetProperty()->GetRepresentation();

  // render points for point picking in a special way
  // all cell types should be rendered as points
  vtkHardwareSelector* selector = ren->GetSelector();
  bool pointPicking = false;
  if (selector &&
      selector->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    representation = VTK_POINTS;
    pointPicking = true;
  }

  // draw points
  if (this->Points.IBO->IndexCount)
  {
    // render points for point picking in a special way
    if (pointPicking)
    {
#if GL_ES_VERSION_2_0 != 1
      glPointSize(2.0);
#endif
    }

    // Update/build/etc the shader.
    this->UpdateShaders(this->Points, ren, actor);
    this->Points.IBO->Bind();
    glDrawRangeElements(GL_POINTS, 0,
                        static_cast<GLuint>(this->VBO->VertexCount - 1),
                        static_cast<GLsizei>(this->Points.IBO->IndexCount),
                        GL_UNSIGNED_INT,
                        reinterpret_cast<const GLvoid *>(NULL));
    this->Points.IBO->Release();
    this->PrimitiveIDOffset += (int)this->Points.IBO->IndexCount;
  }

  // draw lines
  if (this->Lines.IBO->IndexCount)
  {
    this->UpdateShaders(this->Lines, ren, actor);
    if (!this->HaveWideLines(ren,actor))
    {
      glLineWidth(actor->GetProperty()->GetLineWidth());
    }
    this->Lines.IBO->Bind();
    if (representation == VTK_POINTS)
    {
      if (pointPicking)
      {
  #if GL_ES_VERSION_2_0 != 1
        glPointSize(4.0);
  #endif
      }
      glDrawRangeElements(GL_POINTS, 0,
                          static_cast<GLuint>(this->VBO->VertexCount - 1),
                          static_cast<GLsizei>(this->Lines.IBO->IndexCount),
                          GL_UNSIGNED_INT,
                          reinterpret_cast<const GLvoid *>(NULL));
    }
    else
    {
      glDrawRangeElements(GL_LINES, 0,
                          static_cast<GLuint>(this->VBO->VertexCount - 1),
                          static_cast<GLsizei>(this->Lines.IBO->IndexCount),
                          GL_UNSIGNED_INT,
                          reinterpret_cast<const GLvoid *>(NULL));
    }
    this->Lines.IBO->Release();
    this->PrimitiveIDOffset = this->PrimitiveIDOffset +
      static_cast<int>((representation == VTK_POINTS ? this->Lines.IBO->IndexCount
        : this->Lines.IBO->IndexCount/2));
  }

  // draw polygons
  if (this->Tris.IBO->IndexCount)
  {
    // First we do the triangles, update the shader, set uniforms, etc.
    this->UpdateShaders(this->Tris, ren, actor);
    if (!this->HaveWideLines(ren,actor) && representation == VTK_WIREFRAME)
    {
      glLineWidth(actor->GetProperty()->GetLineWidth());
    }
    this->Tris.IBO->Bind();
    GLenum mode = (representation == VTK_POINTS) ? GL_POINTS :
      (representation == VTK_WIREFRAME) ? GL_LINES : GL_TRIANGLES;
    if (pointPicking)
    {
#if GL_ES_VERSION_2_0 != 1
      glPointSize(6.0);
#endif
    }
    glDrawRangeElements(mode, 0,
                      static_cast<GLuint>(this->VBO->VertexCount - 1),
                      static_cast<GLsizei>(this->Tris.IBO->IndexCount),
                      GL_UNSIGNED_INT,
                      reinterpret_cast<const GLvoid *>(NULL));
    this->Tris.IBO->Release();
    this->PrimitiveIDOffset = this->PrimitiveIDOffset +
      static_cast<int>((representation == VTK_POINTS ? this->Tris.IBO->IndexCount
        : (representation == VTK_WIREFRAME ? this->Tris.IBO->IndexCount/2
          : this->Tris.IBO->IndexCount/3)));
  }

  // draw strips
  if (this->TriStrips.IBO->IndexCount)
  {
    // Use the tris shader program/VAO, but triStrips ibo.
    this->UpdateShaders(this->TriStrips, ren, actor);
    this->TriStrips.IBO->Bind();
    if (representation == VTK_POINTS)
    {
      if (pointPicking)
      {
  #if GL_ES_VERSION_2_0 != 1
        glPointSize(6.0);
  #endif
      }
      glDrawRangeElements(GL_POINTS, 0,
                          static_cast<GLuint>(this->VBO->VertexCount - 1),
                          static_cast<GLsizei>(this->TriStrips.IBO->IndexCount),
                          GL_UNSIGNED_INT,
                          reinterpret_cast<const GLvoid *>(NULL));
    }
    if (representation == VTK_WIREFRAME)
    {
      if (!this->HaveWideLines(ren,actor))
      {
        glLineWidth(actor->GetProperty()->GetLineWidth());
      }
      glDrawRangeElements(GL_LINES, 0,
                          static_cast<GLuint>(this->VBO->VertexCount - 1),
                          static_cast<GLsizei>(this->TriStrips.IBO->IndexCount),
                          GL_UNSIGNED_INT,
                          reinterpret_cast<const GLvoid *>(NULL));
    }
    if (representation == VTK_SURFACE)
    {
      glDrawRangeElements(GL_TRIANGLES, 0,
                          static_cast<GLuint>(this->VBO->VertexCount - 1),
                          static_cast<GLsizei>(this->TriStrips.IBO->IndexCount),
                          GL_UNSIGNED_INT,
                          reinterpret_cast<const GLvoid *>(NULL));
    }
    this->TriStrips.IBO->Release();
    this->PrimitiveIDOffset = this->PrimitiveIDOffset +
      static_cast<int>((representation == VTK_POINTS ? this->TriStrips.IBO->IndexCount
        : (representation == VTK_WIREFRAME ? this->TriStrips.IBO->IndexCount/2
          : this->TriStrips.IBO->IndexCount/3)));
  }

  if (selector && (
        selector->GetCurrentPass() == vtkHardwareSelector::ID_LOW24 ||
        selector->GetCurrentPass() == vtkHardwareSelector::ID_MID24 ||
        selector->GetCurrentPass() == vtkHardwareSelector::ID_HIGH16))
  {
    selector->RenderAttributeId(this->PrimitiveIDOffset);
  }
}

//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::RenderPieceFinish(vtkRenderer* ren,
  vtkActor *actor)
{
  vtkHardwareSelector* selector = ren->GetSelector();
  // render points for point picking in a special way
  if (selector &&
      selector->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    glDepthMask(GL_TRUE);
  }
  if (selector && this->PopulateSelectionSettings)
  {
    selector->EndRenderProp();
  }

  if (this->LastBoundBO)
  {
    this->LastBoundBO->VAO->Release();
  }

  this->VBO->Release();

  vtkProperty *prop = actor->GetProperty();
  bool surface_offset =
    (this->GetResolveCoincidentTopology() || prop->GetEdgeVisibility())
    && prop->GetRepresentation() == VTK_SURFACE;
  if (surface_offset)
  {
    glDisable(GL_POLYGON_OFFSET_FILL);
  }

  if (this->ColorTextureMap)
  {
    this->InternalColorTexture->PostRender(ren);
  }

#if GL_ES_VERSION_2_0 != 1 && GL_ES_VERSION_3_0 != 1
  glEndQuery(GL_TIME_ELAPSED);
#endif

  // If the timer is not accurate enough, set it to a small
  // time so that it is not zero
  if (this->TimeToDraw == 0.0)
  {
    this->TimeToDraw = 0.0001;
  }

  if (this->HaveCellScalars || this->HavePickScalars)
  {
    this->CellScalarTexture->Deactivate();
  }
  if (this->HaveCellNormals)
  {
    this->CellNormalTexture->Deactivate();
  }

#if GL_ES_VERSION_2_0 != 1 && GL_ES_VERSION_3_0 != 1
  if (this->ValuePassHelper->GetRenderingMode() == vtkValuePass::FLOATING_POINT)
  {
    this->ValuePassHelper->RenderPieceFinish();
  }
#endif

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

  this->ResourceCallback->RegisterGraphicsResources(
    static_cast<vtkOpenGLRenderWindow *>(ren->GetRenderWindow()));

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

  this->DrawingEdges = true;

  // draw polygons
  if (this->TrisEdges.IBO->IndexCount)
  {
    // First we do the triangles, update the shader, set uniforms, etc.
    this->UpdateShaders(this->TrisEdges, ren, actor);
    if (!this->HaveWideLines(ren,actor))
    {
      glLineWidth(actor->GetProperty()->GetLineWidth());
    }
    this->TrisEdges.IBO->Bind();
    glDrawRangeElements(GL_LINES, 0,
                        static_cast<GLuint>(this->VBO->VertexCount - 1),
                        static_cast<GLsizei>(this->TrisEdges.IBO->IndexCount),
                        GL_UNSIGNED_INT,
                        reinterpret_cast<const GLvoid *>(NULL));
    this->TrisEdges.IBO->Release();
  }

  // draw strips
  if (this->TriStripsEdges.IBO->IndexCount)
  {
    // Use the tris shader program/VAO, but triStrips ibo.
    this->UpdateShaders(this->TriStripsEdges, ren, actor);
    if (!this->HaveWideLines(ren,actor))
    {
      glLineWidth(actor->GetProperty()->GetLineWidth());
    }
    this->TriStripsEdges.IBO->Bind();
    glDrawRangeElements(GL_LINES, 0,
                        static_cast<GLuint>(this->VBO->VertexCount - 1),
                        static_cast<GLsizei>(this->TriStripsEdges.IBO->IndexCount),
                        GL_UNSIGNED_INT,
                        reinterpret_cast<const GLvoid *>(NULL));
    this->TriStripsEdges.IBO->Release();
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
  // Checks for the pass's rendering mode and updates its configuration.
  // Depending on the case, updates the mapper's color mapping or allocates
  // a buffer.
#if GL_ES_VERSION_2_0 != 1 && GL_ES_VERSION_3_0 != 1
  this->ValuePassHelper->UpdateConfiguration(ren, act, this, this->CurrentInput);
#endif

  // Rebuild buffers if needed
  if (this->GetNeedToRebuildBufferObjects(ren,act))
  {
    this->BuildBufferObjects(ren,act);
  }
}

//-------------------------------------------------------------------------
bool vtkOpenGLPolyDataMapper::GetNeedToRebuildBufferObjects(
  vtkRenderer *vtkNotUsed(ren), vtkActor *act)
{
  // first do a coarse check
  if (this->VBOBuildTime < this->GetMTime() ||
      this->VBOBuildTime < act->GetMTime() ||
      this->VBOBuildTime < this->CurrentInput->GetMTime() ||
      this->VBOBuildTime < this->SelectionStateChanged)
  {
    return true;
  }
  return false;
}

// create the cell scalar array adjusted for ogl Cells


void vtkOpenGLPolyDataMapper::AppendCellTextures(
  vtkRenderer *ren,
  vtkActor *,
  vtkCellArray *prims[4],
  int representation,
  std::vector<unsigned char> &newColors,
  std::vector<float> &newNorms,
  vtkPolyData *poly)
{
  // deal with optional pick mapping arrays
  vtkHardwareSelector* selector = ren->GetSelector();
  vtkUnsignedIntArray* mapArray = NULL;
  vtkIdTypeArray* mapArrayId = NULL;
  vtkPointData *pd = poly->GetPointData();
  vtkCellData *cd = poly->GetCellData();
  vtkPoints *points = poly->GetPoints();
  if (selector)
  {
    switch (selector->GetCurrentPass())
    {
      // point data is used for process_pass which seems odd
      case vtkHardwareSelector::PROCESS_PASS:
       if (selector->GetUseProcessIdFromData())
       {
        mapArray = this->ProcessIdArrayName ?
          vtkArrayDownCast<vtkUnsignedIntArray>(
            pd->GetArray(this->ProcessIdArrayName)) : NULL;
       }
        break;
      case vtkHardwareSelector::COMPOSITE_INDEX_PASS:
        mapArray = this->CompositeIdArrayName ?
          vtkArrayDownCast<vtkUnsignedIntArray>(
            cd->GetArray(this->CompositeIdArrayName)) : NULL;
        break;
      case vtkHardwareSelector::ID_LOW24:
      case vtkHardwareSelector::ID_MID24:
        if (selector->GetFieldAssociation() ==
          vtkDataObject::FIELD_ASSOCIATION_POINTS)
        {
          mapArrayId = this->PointIdArrayName ?
            vtkArrayDownCast<vtkIdTypeArray>(
              pd->GetArray(this->PointIdArrayName)) : NULL;
        }
        else
        {
          mapArrayId = this->CellIdArrayName ?
            vtkArrayDownCast<vtkIdTypeArray>(
              cd->GetArray(this->CellIdArrayName)) : NULL;
        }
        break;
    }
  }

  this->HavePickScalars = false;
  if (selector && this->PopulateSelectionSettings &&
      (mapArray ||
        selector->GetCurrentPass() >= vtkHardwareSelector::ID_LOW24))
  {
    this->HavePickScalars = true;
  }

  // handle composite ID point picking seperately as the data is on Cells
  if (this->HavePickScalars &&
      selector->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_POINTS &&
      selector->GetCurrentPass() == vtkHardwareSelector::COMPOSITE_INDEX_PASS)
  {
    std::vector<unsigned char> tmpColors;
    // composite id is stored in ***CELL DATA*** but in point
    // rendering each point of each cell is rendered. So we
    // put the provided value into the texture for each point
    // of each cell
    vtkIdType* indices(NULL);
    vtkIdType npts(0);
    // for each prim type
    vtkIdType cellNum = 0;
    for (int j = 0; j < 4; j++)
    {
      for (prims[j]->InitTraversal(); prims[j]->GetNextCell(npts, indices); )
      {
        unsigned int value = mapArray->GetValue(cellNum);
        value++; // see vtkHardwareSelector.cxx ID_OFFSET
        for (int i = 0; i < npts; i++)
        {
          newColors.push_back(value & 0xff);
          newColors.push_back((value & 0xff00) >> 8);
          newColors.push_back((value & 0xff0000) >> 16);
          newColors.push_back(0xff);
        }
      } // for cell
    }
    return;
  }


  // handle point picking, all is drawn as points
  if (this->HavePickScalars &&
      selector->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    vtkIdType* indices(NULL);
    vtkIdType npts(0);

    for (int j = 0; j < 4; j++)
    {
      for (prims[j]->InitTraversal(); prims[j]->GetNextCell(npts, indices); )
      {
        for (int i=0; i < npts; ++i)
        {
          unsigned int value = indices[i];
          if (mapArrayId)
          {
            value = mapArrayId->GetValue(indices[i]);
          }
          if (mapArray)
          {
            value = mapArray->GetValue(indices[i]);
          }
          value++;
          if (selector->GetCurrentPass() == vtkHardwareSelector::ID_MID24)
          {
            value = (value & 0xff000000) >> 24;
          }
          newColors.push_back(value & 0xff);
          newColors.push_back((value & 0xff00) >> 8);
          newColors.push_back((value & 0xff0000) >> 16);
          newColors.push_back(0xff);
        }
      } // for cell
    }
    return;
  }

  // handle call based process_id picking
  if (this->HavePickScalars &&
      selector->GetCurrentPass() == vtkHardwareSelector::PROCESS_PASS)
  {
    std::vector<unsigned char> tmpColors;
    // process id is stored in point data which if we were not already
    // dealing with cell picking would be fine, but we are, so it makes our
    // job that much harder.  So we first traverse all the cells to
    // find their first point id and then use the point id to
    // lookup a process values. Then we use the map of opengl cells
    // to vtk cells to map into the first array
    vtkIdType* indices(NULL);
    vtkIdType npts(0);
    // for each prim type
    for (int j = 0; j < 4; j++)
    {
      for (prims[j]->InitTraversal(); prims[j]->GetNextCell(npts, indices); )
      {
        unsigned int value = mapArray->GetValue(indices[0]);
        value++;
        tmpColors.push_back(value & 0xff);
        tmpColors.push_back((value & 0xff00) >> 8);
        tmpColors.push_back((value & 0xff0000) >> 16);
        tmpColors.push_back(0xff);
      } // for cell
    }
    // now traverse the opengl to vtk mapping
    std::vector<unsigned int> cellCellMap;
    if (this->HaveAppleBug)
    {
      unsigned int numCells = poly->GetNumberOfCells();
      for (unsigned int i = 0; i < numCells; i++)
      {
        cellCellMap.push_back(i);
      }
    }
    else
    {
      vtkOpenGLIndexBufferObject::CreateCellSupportArrays(
        prims, cellCellMap, representation, points);
    }

    for (unsigned int i = 0; i < cellCellMap.size(); i++)
    {
      unsigned int value = cellCellMap[i];
      newColors.push_back(tmpColors[value*4]);
      newColors.push_back(tmpColors[value*4+1]);
      newColors.push_back(tmpColors[value*4+2]);
      newColors.push_back(tmpColors[value*4+3]);
    }
    return;
  }

  // handle cell based picking
  if (this->HaveCellScalars || this->HaveCellNormals || this->HavePickScalars)
  {
    std::vector<unsigned int> cellCellMap;
    if (this->HaveAppleBug)
    {
      unsigned int numCells = poly->GetNumberOfCells();
      for (unsigned int i = 0; i < numCells; i++)
      {
        cellCellMap.push_back(i);
      }
    }
    else
    {
      vtkOpenGLIndexBufferObject::CreateCellSupportArrays(
        prims, cellCellMap, representation, points);
    }

    if (this->HaveCellScalars || this->HavePickScalars)
    {
      int numComp = 4;

      if (this->HavePickScalars)
      {
        for (unsigned int i = 0; i < cellCellMap.size(); i++)
        {
          unsigned int value = cellCellMap[i];
          if (mapArray)
          {
            value = mapArray->GetValue(value);
          }
          if (mapArrayId)
          {
            value = mapArrayId->GetValue(value);
          }
          value++; // see vtkHardwareSelector.cxx ID_OFFSET
          if (selector->GetCurrentPass() == vtkHardwareSelector::ID_MID24)
          {
            value = (value & 0xff000000) >> 24;
          }
          newColors.push_back(value & 0xff);
          newColors.push_back((value & 0xff00) >> 8);
          newColors.push_back((value & 0xff0000) >> 16);
          newColors.push_back(0xff);
        }
      }
      else
      {
        numComp = this->Colors->GetNumberOfComponents();
        unsigned char *colorPtr = this->Colors->GetPointer(0);
        assert(numComp == 4);
        // use a single color value?
        if (this->FieldDataTupleId > -1 &&
            this->ScalarMode == VTK_SCALAR_MODE_USE_FIELD_DATA)
        {
          for (unsigned int i = 0; i < cellCellMap.size(); i++)
          {
            for (int j = 0; j < numComp; j++)
            {
              newColors.push_back(colorPtr[this->FieldDataTupleId*numComp + j]);
            }
          }
        }
        else
        {
          for (unsigned int i = 0; i < cellCellMap.size(); i++)
          {
            for (int j = 0; j < numComp; j++)
            {
              newColors.push_back(colorPtr[cellCellMap[i]*numComp + j]);
            }
          }
        }
      }
    }

    if (this->HaveCellNormals)
    {
      // create the cell scalar array adjusted for ogl Cells
      vtkDataArray *n = this->CurrentInput->GetCellData()->GetNormals();
      for (unsigned int i = 0; i < cellCellMap.size(); i++)
      {
        // RGB32F requires a later version of OpenGL than 3.2
        // with 3.2 we know we have RGBA32F hence the extra value
        double *norms = n->GetTuple(cellCellMap[i]);
        newNorms.push_back(norms[0]);
        newNorms.push_back(norms[1]);
        newNorms.push_back(norms[2]);
        newNorms.push_back(0);
      }
    }
  }
}

void vtkOpenGLPolyDataMapper::BuildCellTextures(
  vtkRenderer *ren,
  vtkActor *actor,
  vtkCellArray *prims[4],
  int representation)
{
  // create the cell scalar array adjusted for ogl Cells
  std::vector<unsigned char> newColors;
  std::vector<float> newNorms;
  this->AppendCellTextures(ren, actor, prims, representation,
    newColors, newNorms, this->CurrentInput);

  // allocate as needed
  if (this->HaveCellScalars || this->HavePickScalars)
  {
    if (!this->CellScalarTexture)
    {
      this->CellScalarTexture = vtkTextureObject::New();
      this->CellScalarBuffer = vtkOpenGLBufferObject::New();
      this->CellScalarBuffer->SetType(vtkOpenGLBufferObject::TextureBuffer);
    }
    this->CellScalarTexture->SetContext(
      static_cast<vtkOpenGLRenderWindow*>(ren->GetVTKWindow()));
    this->CellScalarBuffer->Upload(newColors,
      vtkOpenGLBufferObject::TextureBuffer);
    this->CellScalarTexture->CreateTextureBuffer(
      static_cast<unsigned int>(newColors.size()/4),
      4,
      VTK_UNSIGNED_CHAR,
      this->CellScalarBuffer);
  }

  if (this->HaveCellNormals)
  {
    if (!this->CellNormalTexture)
    {
      this->CellNormalTexture = vtkTextureObject::New();
      this->CellNormalBuffer = vtkOpenGLBufferObject::New();
      this->CellNormalBuffer->SetType(vtkOpenGLBufferObject::TextureBuffer);
    }
    this->CellNormalTexture->SetContext(
      static_cast<vtkOpenGLRenderWindow*>(ren->GetVTKWindow()));

    // do we have float texture support ?
    int ftex =
      static_cast<vtkOpenGLRenderWindow *>(ren->GetRenderWindow())->
        GetDefaultTextureInternalFormat(VTK_FLOAT, 4, false, true);

    if (ftex)
    {
      this->CellNormalBuffer->Upload(newNorms,
        vtkOpenGLBufferObject::TextureBuffer);
      this->CellNormalTexture->CreateTextureBuffer(
        static_cast<unsigned int>(newNorms.size()/4),
        4, VTK_FLOAT,
        this->CellNormalBuffer);
    }
    else
    {
      // have to convert to unsigned char if no float support
      std::vector<unsigned char> ucNewNorms;
      ucNewNorms.resize(newNorms.size());
      for (size_t i = 0; i < newNorms.size(); i++)
      {
        ucNewNorms[i] = 127.0*(newNorms[i] + 1.0);
      }
      this->CellNormalBuffer->Upload(ucNewNorms,
        vtkOpenGLBufferObject::TextureBuffer);
      this->CellNormalTexture->CreateTextureBuffer(
        static_cast<unsigned int>(newNorms.size()/4),
        4, VTK_UNSIGNED_CHAR,
        this->CellNormalBuffer);
    }
  }
}

// on some apple systems gl_PrimitiveID does not work
// correctly.  So we have to make sure there are no
// shared vertices and build an aray that maps verts
// to their cell id
vtkPolyData *vtkOpenGLPolyDataMapper::HandleAppleBug(
  vtkPolyData *poly,
  std::vector<float> &buffData
  )
{
  vtkIdType* indices = NULL;
  vtkIdType npts = 0;

  vtkPolyData *newPD = vtkPolyData::New();
  newPD->GetCellData()->PassData(poly->GetCellData());
  vtkPoints *points = poly->GetPoints();
  vtkPoints *newPoints = vtkPoints::New();
  newPD->SetPoints(newPoints);
  vtkPointData *pointData = poly->GetPointData();
  vtkPointData *newPointData = newPD->GetPointData();
  newPointData->CopyStructure(pointData);
  newPointData->CopyAllocate(pointData);

  vtkCellArray *prims[4];
  prims[0] =  poly->GetVerts();
  prims[1] =  poly->GetLines();
  prims[2] =  poly->GetPolys();
  prims[3] =  poly->GetStrips();

  // build a new PolyData with no shared cells

  // for each prim type
  unsigned int newPointCount = 0;
  buffData.reserve(points->GetNumberOfPoints());
  for (int j = 0; j < 4; j++)
  {
    unsigned int newCellCount = 0;
    if (prims[j]->GetNumberOfCells())
    {
      vtkCellArray *ca = vtkCellArray::New();
      switch (j)
      {
        case 0: newPD->SetVerts(ca); break;
        case 1: newPD->SetLines(ca); break;
        case 2: newPD->SetPolys(ca); break;
        case 3: newPD->SetStrips(ca); break;
      }

      // foreach cell
      for (prims[j]->InitTraversal(); prims[j]->GetNextCell(npts, indices); )
      {
        ca->InsertNextCell(npts);
        vtkucfloat c;
        c.c[0] = newCellCount&0xff;
        c.c[1] = (newCellCount >> 8)&0xff;
        c.c[2] = (newCellCount >> 16)&0xff;
        c.c[3] =  0;
        for (int i=0; i < npts; ++i)
        {
          // insert point data
          newPoints->InsertNextPoint(points->GetPoint(indices[i]));
          ca->InsertCellPoint(newPointCount);
          newPointData->CopyData(pointData,indices[i],newPointCount);
          buffData.push_back(c.f);
          newPointCount++;
        }
        newCellCount++;
      }
      ca->Delete();
    }
  }

  newPoints->Delete();
  return newPD;
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

  this->HaveCellScalars = false;
  vtkDataArray *c = this->Colors;
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
      this->HaveCellScalars = true;
      c = NULL;
    }
  }

  this->HaveCellNormals = false;
  // Do we have cell normals?
  vtkDataArray *n =
    (act->GetProperty()->GetInterpolation() != VTK_FLAT) ? poly->GetPointData()->GetNormals() : NULL;
  if (n == NULL && poly->GetCellData()->GetNormals())
  {
    this->HaveCellNormals = true;
  }

  int representation = act->GetProperty()->GetRepresentation();
  vtkHardwareSelector* selector = ren->GetSelector();
  bool pointPicking = false;
  if (selector &&
      selector->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    representation = VTK_POINTS;
    pointPicking = true;
  }

  // check if this system is subject to the apple/amd primID bug
  this->HaveAppleBug =
    static_cast<vtkOpenGLRenderer *>(ren)->HaveApplePrimitiveIdBug();
  if (this->HaveAppleBugForce == 1)
  {
    this->HaveAppleBug = false;
  }
  if (this->HaveAppleBugForce == 2)
  {
    this->HaveAppleBug = true;
  }

  vtkCellArray *prims[4];
  prims[0] =  poly->GetVerts();
  prims[1] =  poly->GetLines();
  prims[2] =  poly->GetPolys();
  prims[3] =  poly->GetStrips();

  // only rebuild what we need to
  // if the data or mapper or selection state changed
  // then rebuild the cell arrays
  std::ostringstream toString;
  toString.str("");
  toString.clear();
  toString << (prims[0]->GetNumberOfCells() ? prims[0]->GetMTime() : 0) <<
    'A' << (prims[1]->GetNumberOfCells() ? prims[1]->GetMTime() : 0) <<
    'B' << (prims[2]->GetNumberOfCells() ? prims[2]->GetMTime() : 0) <<
    'C' << (prims[3]->GetNumberOfCells() ? prims[3]->GetMTime() : 0) <<
    'D' << representation <<
    'E' << this->LastSelectionState <<
    'F' << poly->GetMTime() <<
    'G' << this->GetMTime();
  if (this->CellTextureBuildString != toString.str())
  {
    this->BuildCellTextures(ren, act, prims, representation);
    this->CellTextureBuildString = toString.str();
  }

  // on apple with the AMD PrimID bug we use a slow
  // painful approach to work around it
  this->AppleBugPrimIDs.resize(0);
  if (this->HaveAppleBug &&
      !pointPicking &&
      (this->HaveCellNormals || this->HaveCellScalars || this->HavePickScalars))
  {
    if (!this->AppleBugPrimIDBuffer)
    {
      this->AppleBugPrimIDBuffer = vtkOpenGLBufferObject::New();
    }
    poly = this->HandleAppleBug(poly, this->AppleBugPrimIDs);
    this->AppleBugPrimIDBuffer->Bind();
    this->AppleBugPrimIDBuffer->Upload(
     this->AppleBugPrimIDs, vtkOpenGLBufferObject::ArrayBuffer);
    this->AppleBugPrimIDBuffer->Release();

#ifndef NDEBUG
    static bool warnedAboutBrokenAppleDriver = false;
    if (!warnedAboutBrokenAppleDriver)
    {
      vtkWarningMacro("VTK is working around a bug in Apple-AMD hardware related to gl_PrimitiveID.  This may cause significant memory and performance impacts. Your hardware has been identified as vendor "
        << (const char *)glGetString(GL_VENDOR) << " with renderer of "
        << (const char *)glGetString(GL_RENDERER) << " and version "
        << (const char *)glGetString(GL_VERSION));
      warnedAboutBrokenAppleDriver = true;
    }
#endif
    if (n)
    {
      n = (act->GetProperty()->GetInterpolation() != VTK_FLAT) ?
            poly->GetPointData()->GetNormals() : NULL;
    }
    if (c)
    {
      this->Colors->Delete();
      this->Colors = 0;
      this->MapScalars(poly,1.0);
      c = this->Colors;
    }
  }

    // Set the texture if we are going to use texture
    // for coloring with a point attribute.
    vtkDataArray *tcoords = NULL;
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

  // rebuild the VBO if the data has changed we create a string for the VBO what
  // can change the VBO? points normals tcoords colors so what can change those?
  // the input data is clearly one as it can change all four items tcoords may
  // haveTextures or not colors may change based on quite a few mapping
  // parameters in the mapper
  toString.str("");
  toString.clear();
  toString << poly->GetMTime() <<
    'A' << (c ? c->GetMTime() : 1) <<
    'B' << (n ? n->GetMTime() : 1) <<
    'C' << (tcoords ? tcoords->GetMTime() : 1);

  if (this->VBOBuildString != toString.str())
  {
    // Build the VBO
    this->VBO->CreateVBO(poly->GetPoints(),
        poly->GetPoints()->GetNumberOfPoints(),
        n, tcoords,
        c ? (unsigned char *)c->GetVoidPointer(0) : NULL,
        c ? c->GetNumberOfComponents() : 0);

    // If the VBO coordinates were shifted and scaled, prepare the inverse transform
    // for application to the model->view matrix:
    if (this->VBO->GetCoordShiftAndScaleEnabled())
    {
      double shift[3];
      double scale[3];
      this->VBO->GetCoordShift(shift);
      this->VBO->GetCoordScale(scale);
      this->VBOInverseTransform->Identity();
      this->VBOInverseTransform->Translate(shift[0], shift[1], shift[2]);
      this->VBOInverseTransform->Scale(1.0/scale[0], 1.0/scale[1], 1.0/scale[2]);
      this->VBOInverseTransform->GetTranspose(this->VBOShiftScale.GetPointer());
    }
    this->VBOBuildTime.Modified();
    this->VBOBuildString = toString.str();
  }

  // now create the IBOs
  this->BuildIBO(ren, act, poly);

  // free up polydata if allocated due to apple bug
  if (poly != this->CurrentInput)
  {
    poly->Delete();
  }

  vtkOpenGLCheckErrorMacro("failed after BuildBufferObjects");
}

//-------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::BuildIBO(
  vtkRenderer *ren,
  vtkActor *act,
  vtkPolyData *poly)
{
  vtkCellArray *prims[4];
  prims[0] =  poly->GetVerts();
  prims[1] =  poly->GetLines();
  prims[2] =  poly->GetPolys();
  prims[3] =  poly->GetStrips();
  int representation = act->GetProperty()->GetRepresentation();

  vtkHardwareSelector* selector = ren->GetSelector();

  if (selector &&
      selector->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    representation = VTK_POINTS;
  }

  vtkDataArray *ef = poly->GetPointData()->GetAttribute(
                    vtkDataSetAttributes::EDGEFLAG);
  vtkProperty *prop = act->GetProperty();

  bool draw_surface_with_edges =
    (prop->GetEdgeVisibility() && prop->GetRepresentation() == VTK_SURFACE);

  // do we realy need to rebuild the IBO? Since the operation is costly we
  // construst a string of values that impact the IBO and see if that string has
  // changed

  // So...polydata can return a dummy CellArray when there are no lines
  std::ostringstream toString;
  toString.str("");
  toString.clear();
  toString << (prims[0]->GetNumberOfCells() ? prims[0]->GetMTime() : 0) <<
    'A' << (prims[1]->GetNumberOfCells() ? prims[1]->GetMTime() : 0) <<
    'B' << (prims[2]->GetNumberOfCells() ? prims[2]->GetMTime() : 0) <<
    'C' << (prims[3]->GetNumberOfCells() ? prims[3]->GetMTime() : 0) <<
    'D' << representation <<
    'E' << (ef ? ef->GetMTime() : 0) <<
    'F' << draw_surface_with_edges;

  if (this->IBOBuildString != toString.str())
  {
    this->Points.IBO->CreatePointIndexBuffer(prims[0]);

    if (representation == VTK_POINTS)
    {
      this->Lines.IBO->CreatePointIndexBuffer(prims[1]);
      this->Tris.IBO->CreatePointIndexBuffer(prims[2]);
      this->TriStrips.IBO->CreatePointIndexBuffer(prims[3]);
    }
    else // WIREFRAME OR SURFACE
    {
      this->Lines.IBO->CreateLineIndexBuffer(prims[1]);

      if (representation == VTK_WIREFRAME)
      {
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
          this->Tris.IBO->CreateEdgeFlagIndexBuffer(prims[2], ef);
        }
        else
        {
          this->Tris.IBO->CreateTriangleLineIndexBuffer(prims[2]);
        }
        this->TriStrips.IBO->CreateStripIndexBuffer(prims[3], true);
      }
     else // SURFACE
     {
        this->Tris.IBO->CreateTriangleIndexBuffer(prims[2], poly->GetPoints());
        this->TriStrips.IBO->CreateStripIndexBuffer(prims[3], false);
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
          ef = NULL;
        }
        else if (!ef->IsA("vtkUnsignedCharArray"))
        {
          vtkDebugMacro(<< "Currently only unsigned char edge flags are suported.");
          ef = NULL;
        }
      }
      if (ef)
      {
        this->TrisEdges.IBO->CreateEdgeFlagIndexBuffer(prims[2], ef);
      }
      else
      {
        this->TrisEdges.IBO->CreateTriangleLineIndexBuffer(prims[2]);
      }
      this->TriStripsEdges.IBO->CreateStripIndexBuffer(prims[3], true);
    }

    this->IBOBuildString = toString.str();
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

//----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::ShallowCopy(vtkAbstractMapper *mapper)
{
  vtkOpenGLPolyDataMapper *m = vtkOpenGLPolyDataMapper::SafeDownCast(mapper);
  if (m != NULL)
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
  this->VBO->SetCoordShiftAndScaleMethod(
    static_cast<vtkOpenGLVertexBufferObject::ShiftScaleMethod>(m));
}

//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
