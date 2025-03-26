// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkArrayRenderer.h"

#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkColorTransferFunction.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkGLSLModCamera.h"
#include "vtkGLSLModCoincidentTopology.h"
#include "vtkGLSLModLight.h"
#include "vtkGLSLModPixelDebugger.h"
#include "vtkGLSLModifierFactory.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkMatrix3x3.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLActor.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLIndexBufferObject.h"
#include "vtkOpenGLRenderPass.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLState.h"
#include "vtkOpenGLTexture.h"
#include "vtkOpenGLUniforms.h"
#include "vtkOpenGLVertexArrayObject.h"
// #include "vtkOpenGLVertexBufferObject.h"
// #include "vtkOpenGLVertexBufferObjectGroup.h"
#include "vtkPointData.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkShader.h"
#include "vtkShaderProgram.h"
#include "vtkShaderProperty.h"
#include "vtkTexture.h"
#include "vtkTextureObject.h"
#include "vtkTypeInt32Array.h"
#include "vtkTypeUInt32Array.h"
#include "vtkUnsignedCharArray.h"

#include "vtk_glew.h"

#include <numeric> // for std::iota()

VTK_ABI_NAMESPACE_BEGIN

namespace
{
void ReplaceShaderRenderPass(std::string& vsSrc, std::string& gsSrc, std::string& fsSrc,
  vtkAbstractMapper* mapper, vtkActor* actor, bool prePass)
{
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
        if (!rp->PreReplaceShaderValues(vsSrc, gsSrc, fsSrc, mapper, actor))
        {
          vtkErrorWithObjectMacro(
            mapper, "vtkOpenGLRenderPass::ReplaceShaderValues failed for " << rp->GetClassName());
        }
      }
      else
      {
        if (!rp->PostReplaceShaderValues(vsSrc, gsSrc, fsSrc, mapper, actor))
        {
          vtkErrorWithObjectMacro(
            mapper, "vtkOpenGLRenderPass::ReplaceShaderValues failed for " << rp->GetClassName());
        }
      }
    }
  }
}

}

vtkStandardNewMacro(vtkArrayRenderer);

vtkArrayRenderer::vtkArrayRenderer()
{
  this->ResetModsToDefault();
}

void vtkArrayRenderer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  // Most of these ivars are inherited from vtkDrawTexturedElements.
  os << indent << "NumberOfInstances: " << this->NumberOfInstances << "\n";
  os << indent << "NumberOfElements: " << this->NumberOfElements << "\n";
  os << indent << "ElementType: " << this->ElementType << "\n";
  os << indent << "Arrays: " << this->Arrays.size() << "\n";
  int ii = 0;
  vtkIndent i2 = indent.GetNextIndent();
  for (const auto& array : this->Arrays)
  {
    os << i2 << ii << ": " << array.first.Data() << " = " << array.second.Arrays.front()->GetName()
       << " " << array.second.Texture << " " << array.second.Buffer << "\n";
    ++ii;
  }
  os << indent << "ShaderProgram: " << this->ShaderProgram << "\n";
}

void vtkArrayRenderer::PrepareColormap(vtkScalarsToColors* cmap)
{
  if (!cmap && this->ColorTextureMap)
  {
    // We have a previous colormap. Use it.
    return;
  }
  vtkNew<vtkColorTransferFunction> ctf;
  if (!cmap)
  {
    // Create a cool-to-warm (blue to red) diverging colormap by default:
    ctf->SetVectorModeToMagnitude();
    ctf->SetColorSpaceToDiverging();
    ctf->AddRGBPoint(0.0, 59. / 255., 76. / 255., 192. / 255.);
    ctf->AddRGBPoint(0.5, 221. / 255., 221. / 255., 221. / 255.);
    ctf->AddRGBPoint(1.0, 180. / 255., 4. / 255., 38. / 255.);
    ctf->Build();
    cmap = ctf;
  }
  // Now, if there is no colormap texture, make one from the colormap
  if (!this->LookupTable || this->LookupTable->GetMTime() < cmap->GetMTime())
  {
    this->SetLookupTable(cmap);
  }
  if (!this->ColorTextureMap || this->ColorTextureMap->GetMTime() < this->LookupTable->GetMTime())
  {
    this->CreateColormapTexture(); // populate this->ColorTexture from this->LookupTable
  }
}

double* vtkArrayRenderer::GetBounds()
{
  // TODO: FIXME
  // How should we determine bounds? Accept a lambda that is passed the mapper's input?
  // Since the shaders can do anything, we cannot just assume the mapper input will be
  // rendered precisely.
  static std::array<double, 6> bds{ -1.0, +1.0, -1.0, +1.0, -1.0, +1.0 };
  return bds.data();
}

// When new default mods are added, make sure to register them in
// vtkArrayRenderer::ResetModsToDefault below.
std::vector<std::string> vtkArrayRenderer::DefaultModNames = { "vtkGLSLModCamera",
  "vtkGLSLModLight", "vtkGLSLModCoincidentTopology", "vtkGLSLModPixelDebugger" };

void vtkArrayRenderer::ResetModsToDefault()
{
  // just to be sure.
  this->RemoveAllMods();
  this->AddMods(vtkArrayRenderer::DefaultModNames);
  vtkGLSLModifierFactory::RegisterAMod(
    DefaultModNames[0], [](void*) { return vtkGLSLModCamera::New(); });
  vtkGLSLModifierFactory::RegisterAMod(
    DefaultModNames[1], [](void*) { return vtkGLSLModLight::New(); });
  vtkGLSLModifierFactory::RegisterAMod(
    DefaultModNames[2], [](void*) { return vtkGLSLModCoincidentTopology::New(); });
  vtkGLSLModifierFactory::RegisterAMod(
    DefaultModNames[3], [](void*) { return vtkGLSLModPixelDebugger::New(); });
}

void vtkArrayRenderer::AddMod(const std::string& className)
{
  if (!this->ModNamesUnique.count(className))
  {
    this->ModNames.emplace_back(className);
  }
}

void vtkArrayRenderer::AddMods(const std::vector<std::string>& classNames)
{
  for (const auto& modName : classNames)
  {
    this->AddMod(modName);
  }
}

void vtkArrayRenderer::RemoveMod(const std::string& className)
{
  if (this->ModNamesUnique.count(className))
  {
    this->ModNamesUnique.erase(className);
    this->ModNames.erase(
      std::remove(this->ModNames.begin(), this->ModNames.end(), className), this->ModNames.end());
  }
}

void vtkArrayRenderer::RemoveAllMods()
{
  this->ModNamesUnique.clear();
  this->ModNames.clear();
}

bool vtkArrayRenderer::IsUpToDate(vtkRenderer* renderer, vtkActor* actor)
{
  if (this->RenderTimeStamp < actor->GetProperty()->GetMTime())
  {
    return false;
  }
  if (this->RenderTimeStamp < this->GetMTime())
  {
    return false;
  }

  auto modsIter = vtk::TakeSmartPointer(this->GetGLSLModCollection()->NewIterator());
  auto oglRen = static_cast<vtkOpenGLRenderer*>(renderer);
  for (modsIter->InitTraversal(); !modsIter->IsDoneWithTraversal(); modsIter->GoToNextItem())
  {
    auto mod = static_cast<vtkGLSLModifierBase*>(modsIter->GetCurrentObject());
    if (!mod->IsUpToDate(oglRen, this, actor))
    {
      vtkDebugWithObjectMacro(nullptr, << mod->GetClassName() << " is outdated");
      // if any mod is outdated, entire shader program must be re-compiled.
      return false;
    }
  }
  return true;
}

void vtkArrayRenderer::PrepareToRender(vtkRenderer* renderer, vtkActor* actor)
{
  auto* vertShader = this->GetShader(vtkShader::Vertex);
  auto* fragShader = this->GetShader(vtkShader::Fragment);

  std::string vertShaderSource = this->VertexShaderSource;
  std::string emptyGS, emptyTCS, emptyTES;
  std::string fragShaderSource = this->FragmentShaderSource;

  auto* oglRenderer = vtkOpenGLRenderer::SafeDownCast(renderer);
  ::ReplaceShaderRenderPass(vertShaderSource, emptyGS, fragShaderSource, this, actor, true);
  // Apply shader mods.
  this->GetGLSLModCollection()->RemoveAllItems();
  for (const auto& modName : this->ModNames)
  {
    auto mod = vtk::TakeSmartPointer(vtkGLSLModifierFactory::CreateAMod(modName));
    mod->ReplaceShaderValues(
      oglRenderer, vertShaderSource, emptyTCS, emptyTES, emptyGS, fragShaderSource, this, actor);
    this->GetGLSLModCollection()->AddItem(mod);
  }
  // Post-pass.
  ::ReplaceShaderRenderPass(vertShaderSource, emptyGS, fragShaderSource, this, actor, false);

  vertShader->SetSource(vertShaderSource);
  fragShader->SetSource(fragShaderSource);
}

void vtkArrayRenderer::Render(vtkRenderer* ren, vtkActor* actor)
{
  if (!this->IsUpToDate(ren, actor))
  {
    this->PrepareToRender(ren, actor);
  }
  this->DrawInstancedElements(ren, actor, this);
  this->RenderTimeStamp.Modified();
}

void vtkArrayRenderer::ReleaseGraphicsResources(vtkWindow* window)
{
  this->ReleaseResources(window);
}

int vtkArrayRenderer::FillInputPortInformation(int port, vtkInformation* info)
{
  (void)port;
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}

void vtkArrayRenderer::CreateColormapTexture()
{
  if (!this->LookupTable)
  {
    if (this->ColorTextureMap)
    {
      this->ColorTextureMap->UnRegister(this);
      this->ColorTextureMap = nullptr;
    }
    return;
  }

  // Can we use the texture we already have?
  if (this->ColorTextureMap && this->GetMTime() < this->ColorTextureMap->GetMTime() &&
    this->LookupTable->GetMTime() < this->ColorTextureMap->GetMTime())
  {
    return;
  }

  // Nope, allocate one if needed.
  if (!this->ColorTextureMap)
  {
    this->ColorTextureMap = vtkImageData::New();
  }

  double* range = this->LookupTable->GetRange();
  // Get the texture map from the lookup table.
  // Create a dummy ramp of scalars.
  // In the future, we could extend vtkScalarsToColors.
  vtkIdType numberOfColors = this->LookupTable->GetNumberOfAvailableColors();
  numberOfColors += 2;
  // number of available colors can return 2^24
  // which is an absurd size for a tmap in this case. So we
  // watch for cases like that and reduce it to a
  // more reasonable size
  if (numberOfColors > 65538) // 65536+2
  {
    numberOfColors = 8192;
  }
  double k = (range[1] - range[0]) / (numberOfColors - 2);
  vtkNew<vtkDoubleArray> tmp;
  tmp->SetNumberOfTuples(numberOfColors * 2);
  double* ptr = tmp->GetPointer(0);
  bool use_log_scale = false; // FIXME
  for (int i = 0; i < numberOfColors; ++i)
  {
    *ptr = range[0] + i * k - k / 2.0; // minus k / 2 to start at below range color
    if (use_log_scale)
    {
      *ptr = pow(10.0, *ptr);
    }
    ++ptr;
  }
  // Dimension on NaN.
  double nan = vtkMath::Nan();
  for (int i = 0; i < numberOfColors; ++i)
  {
    *ptr = nan;
    ++ptr;
  }
  this->ColorTextureMap->SetExtent(0, numberOfColors - 1, 0, 1, 0, 0);
  this->ColorTextureMap->GetPointData()->SetScalars(
    this->LookupTable->MapScalars(tmp, this->ColorMode, 0));
  // this->LookupTable->SetAlpha(orig_alpha);
  this->ColorTextureMap->GetPointData()->GetScalars()->Delete();
}

VTK_ABI_NAMESPACE_END
