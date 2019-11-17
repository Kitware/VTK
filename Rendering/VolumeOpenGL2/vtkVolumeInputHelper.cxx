#include <sstream>

#include "vtkColorTransferFunction.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkOpenGLGPUVolumeRayCastMapper.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLVolumeGradientOpacityTable.h"
#include "vtkOpenGLVolumeOpacityTable.h"
#include "vtkOpenGLVolumeRGBTable.h"
#include "vtkOpenGLVolumeTransferFunction2D.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRenderer.h"
#include "vtkShaderProgram.h"
#include "vtkTextureObject.h"
#include "vtkVolume.h"
#include "vtkVolumeInputHelper.h"
#include "vtkVolumeProperty.h"
#include "vtkVolumeTexture.h"

vtkVolumeInputHelper::vtkVolumeInputHelper(vtkSmartPointer<vtkVolumeTexture> tex, vtkVolume* vol)
  : Texture(tex)
  , Volume(vol)
{
}

void vtkVolumeInputHelper::RefreshTransferFunction(
  vtkRenderer* ren, const int uniformIndex, const int blendMode, const float samplingDist)
{
  if (this->InitializeTransfer ||
    this->Volume->GetProperty()->GetMTime() > this->LutInit.GetMTime())
  {
    this->InitializeTransferFunction(ren, uniformIndex);
  }
  this->UpdateTransferFunctions(ren, blendMode, samplingDist);
}

void vtkVolumeInputHelper::InitializeTransferFunction(vtkRenderer* ren, const int index)
{
  const int transferMode = this->Volume->GetProperty()->GetTransferFunctionMode();
  switch (transferMode)
  {
    case vtkVolumeProperty::TF_2D:
      this->CreateTransferFunction2D(ren, index);
      break;

    case vtkVolumeProperty::TF_1D:
    default:
      this->CreateTransferFunction1D(ren, index);
  }
  this->InitializeTransfer = false;
}

void vtkVolumeInputHelper::UpdateTransferFunctions(
  vtkRenderer* ren, const int blendMode, const float samplingDist)
{
  auto vol = this->Volume;
  const int transferMode = vol->GetProperty()->GetTransferFunctionMode();
  const int numComp = this->Texture->GetLoadedScalars()->GetNumberOfComponents();
  switch (transferMode)
  {
    case vtkVolumeProperty::TF_1D:
      switch (this->ComponentMode)
      {
        case vtkVolumeInputHelper::INDEPENDENT:
          for (int i = 0; i < numComp; ++i)
          {
            this->UpdateOpacityTransferFunction(ren, vol, i, blendMode, samplingDist);
            this->UpdateGradientOpacityTransferFunction(ren, vol, i, samplingDist);
            this->UpdateColorTransferFunction(ren, vol, i);
          }
          break;
        default: // RGBA or LA
          this->UpdateOpacityTransferFunction(ren, vol, numComp - 1, blendMode, samplingDist);
          this->UpdateGradientOpacityTransferFunction(ren, vol, numComp - 1, samplingDist);
          this->UpdateColorTransferFunction(ren, vol, 0);
      }
      break;

    case vtkVolumeProperty::TF_2D:
      switch (this->ComponentMode)
      {
        case vtkVolumeInputHelper::INDEPENDENT:
          for (int i = 0; i < numComp; ++i)
          {
            this->UpdateTransferFunction2D(ren, i);
          }
          break;
        default: // RGBA or LA
          this->UpdateTransferFunction2D(ren, 0);
      }
      break;
  }
}

int vtkVolumeInputHelper::UpdateOpacityTransferFunction(vtkRenderer* ren, vtkVolume* vol,
  unsigned int component, const int blendMode, const float samplingDist)
{
  vtkVolumeProperty* volumeProperty = vol->GetProperty();

  // Use the first LUT when using dependent components
  unsigned int lookupTableIndex = volumeProperty->GetIndependentComponents() ? component : 0;
  vtkPiecewiseFunction* scalarOpacity = volumeProperty->GetScalarOpacity(lookupTableIndex);

  auto volumeTex = this->Texture.GetPointer();
  double componentRange[2];
  if (scalarOpacity->GetSize() < 1 ||
    this->ScalarOpacityRangeType == vtkGPUVolumeRayCastMapper::SCALAR)
  {
    for (int i = 0; i < 2; ++i)
    {
      componentRange[i] = volumeTex->ScalarRange[component][i];
    }
  }
  else
  {
    scalarOpacity->GetRange(componentRange);
  }

  if (scalarOpacity->GetSize() < 1)
  {
    scalarOpacity->AddPoint(componentRange[0], 0.0);
    scalarOpacity->AddPoint(componentRange[1], 0.5);
  }

  int filterVal = volumeProperty->GetInterpolationType() == VTK_LINEAR_INTERPOLATION
    ? vtkTextureObject::Linear
    : vtkTextureObject::Nearest;

  this->OpacityTables->GetTable(lookupTableIndex)
    ->Update(scalarOpacity, componentRange, blendMode, samplingDist,
      volumeProperty->GetScalarOpacityUnitDistance(component),
#ifndef GL_ES_VERSION_3_0
      filterVal,
#else
      vtkTextureObject::Nearest,
#endif
      vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow()));

  return 0;
}

int vtkVolumeInputHelper::UpdateColorTransferFunction(
  vtkRenderer* ren, vtkVolume* vol, unsigned int component)
{
  vtkVolumeProperty* volumeProperty = vol->GetProperty();

  // Build the colormap in a 1D texture. 1D RGB-texture-mapping from scalar
  // values to color values build the table.
  vtkColorTransferFunction* colorTransferFunction =
    volumeProperty->GetRGBTransferFunction(component);

  auto volumeTex = this->Texture.GetPointer();
  double componentRange[2];
  if (colorTransferFunction->GetSize() < 1 ||
    this->ColorRangeType == vtkGPUVolumeRayCastMapper::SCALAR)
  {
    for (int i = 0; i < 2; ++i)
    {
      componentRange[i] = volumeTex->ScalarRange[component][i];
    }
  }
  else
  {
    colorTransferFunction->GetRange(componentRange);
  }

  // Add points only if its not being added before
  if (colorTransferFunction->GetSize() < 1)
  {
    colorTransferFunction->AddRGBPoint(componentRange[0], 0.0, 0.0, 0.0);
    colorTransferFunction->AddRGBPoint(componentRange[1], 1.0, 1.0, 1.0);
  }

  int filterVal = volumeProperty->GetInterpolationType() == VTK_LINEAR_INTERPOLATION
    ? vtkTextureObject::Linear
    : vtkTextureObject::Nearest;

  this->RGBTables->GetTable(component)->Update(volumeProperty->GetRGBTransferFunction(component),
    componentRange, 0, 0, 0,
#ifndef GL_ES_VERSION_3_0
    filterVal,
#else
    vtkTextureObject::Nearest,
#endif
    vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow()));

  return 0;
}

int vtkVolumeInputHelper::UpdateGradientOpacityTransferFunction(
  vtkRenderer* ren, vtkVolume* vol, unsigned int component, const float samplingDist)
{
  vtkVolumeProperty* volumeProperty = vol->GetProperty();

  // Use the first LUT when using dependent components
  unsigned int lookupTableIndex = volumeProperty->GetIndependentComponents() ? component : 0;

  if (!volumeProperty->HasGradientOpacity(lookupTableIndex) || !this->GradientOpacityTables)
  {
    return 1;
  }

  vtkPiecewiseFunction* gradientOpacity = volumeProperty->GetGradientOpacity(lookupTableIndex);

  auto volumeTex = this->Texture.GetPointer();
  double componentRange[2];
  if (gradientOpacity->GetSize() < 1 ||
    this->GradientOpacityRangeType == vtkGPUVolumeRayCastMapper::SCALAR)
  {
    for (int i = 0; i < 2; ++i)
    {
      componentRange[i] = volumeTex->ScalarRange[component][i];
    }
  }
  else
  {
    gradientOpacity->GetRange(componentRange);
  }

  if (gradientOpacity->GetSize() < 1)
  {
    gradientOpacity->AddPoint(componentRange[0], 0.0);
    gradientOpacity->AddPoint(componentRange[1], 0.5);
  }

  int filterVal = volumeProperty->GetInterpolationType() == VTK_LINEAR_INTERPOLATION
    ? vtkTextureObject::Linear
    : vtkTextureObject::Nearest;

  this->GradientOpacityTables->GetTable(lookupTableIndex)
    ->Update(gradientOpacity, componentRange, 0, samplingDist,
      volumeProperty->GetScalarOpacityUnitDistance(component),
#ifndef GL_ES_VERSION_3_0
      filterVal,
#else
      vtkTextureObject::Nearest,
#endif
      vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow()));

  return 0;
}

void vtkVolumeInputHelper::UpdateTransferFunction2D(vtkRenderer* ren, unsigned int component)
{
  // Use the first LUT when using dependent components
  vtkVolumeProperty* prop = this->Volume->GetProperty();
  unsigned int const lutIndex = prop->GetIndependentComponents() ? component : 0;

  vtkImageData* transfer2D = prop->GetTransferFunction2D(lutIndex);
#ifndef GL_ES_VERSION_3_0
  int const interp = prop->GetInterpolationType() == VTK_LINEAR_INTERPOLATION
    ? vtkTextureObject::Linear
    : vtkTextureObject::Nearest;
#else
  int const interp = vtkTextureObject::Nearest;
#endif

  double scalarRange[2] = { 0, 1 };
  this->TransferFunctions2D->GetTable(lutIndex)->Update(transfer2D, scalarRange, 0, 0, 0, interp,
    vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow()));
}

void vtkVolumeInputHelper::ActivateTransferFunction(vtkShaderProgram* prog, const int blendMode)
{
  int const transferMode = this->Volume->GetProperty()->GetTransferFunctionMode();
  int const numActiveLuts =
    this->ComponentMode == INDEPENDENT ? Texture->GetLoadedScalars()->GetNumberOfComponents() : 1;
  switch (transferMode)
  {
    case vtkVolumeProperty::TF_1D:
      for (int i = 0; i < numActiveLuts; ++i)
      {
        this->OpacityTables->GetTable(i)->Activate();
        prog->SetUniformi(
          this->OpacityTablesMap[i].c_str(), this->OpacityTables->GetTable(i)->GetTextureUnit());

        if (blendMode != vtkGPUVolumeRayCastMapper::ADDITIVE_BLEND)
        {
          this->RGBTables->GetTable(i)->Activate();
          prog->SetUniformi(
            this->RGBTablesMap[i].c_str(), this->RGBTables->GetTable(i)->GetTextureUnit());
        }

        if (this->GradientOpacityTables)
        {
          this->GradientOpacityTables->GetTable(i)->Activate();
          prog->SetUniformi(this->GradientOpacityTablesMap[i].c_str(),
            this->GradientOpacityTables->GetTable(i)->GetTextureUnit());
        }
      }
      break;
    case vtkVolumeProperty::TF_2D:
      for (int i = 0; i < numActiveLuts; ++i)
      {
        vtkOpenGLVolumeTransferFunction2D* table = this->TransferFunctions2D->GetTable(i);
        table->Activate();
        prog->SetUniformi(this->TransferFunctions2DMap[i].c_str(), table->GetTextureUnit());
      }
      break;
  }
}

void vtkVolumeInputHelper::DeactivateTransferFunction(const int blendMode)
{
  int const transferMode = this->Volume->GetProperty()->GetTransferFunctionMode();
  int const numActiveLuts =
    this->ComponentMode == INDEPENDENT ? Texture->GetLoadedScalars()->GetNumberOfComponents() : 1;
  switch (transferMode)
  {
    case vtkVolumeProperty::TF_1D:
      for (int i = 0; i < numActiveLuts; ++i)
      {
        this->OpacityTables->GetTable(i)->Deactivate();
        if (blendMode != vtkGPUVolumeRayCastMapper::ADDITIVE_BLEND)
        {
          this->RGBTables->GetTable(i)->Deactivate();
        }
        if (this->GradientOpacityTables)
        {
          this->GradientOpacityTables->GetTable(i)->Deactivate();
        }
      }
      break;
    case vtkVolumeProperty::TF_2D:
      for (int i = 0; i < numActiveLuts; ++i)
      {
        this->TransferFunctions2D->GetTable(i)->Deactivate();
      }
      break;
  }
}

void vtkVolumeInputHelper::CreateTransferFunction1D(vtkRenderer* ren, const int index)
{
  this->ReleaseGraphicsTransfer1D(ren->GetRenderWindow());

  int const numActiveLuts =
    this->ComponentMode == INDEPENDENT ? Texture->GetLoadedScalars()->GetNumberOfComponents() : 1;

  // Create RGB and opacity (scalar and gradient) lookup tables. Up to four
  // components are supported in single-input independentComponents mode.
  this->RGBTables = vtkSmartPointer<vtkOpenGLVolumeLookupTables<vtkOpenGLVolumeRGBTable> >::New();
  this->RGBTables->Create(numActiveLuts);
  this->OpacityTables =
    vtkSmartPointer<vtkOpenGLVolumeLookupTables<vtkOpenGLVolumeOpacityTable> >::New();
  this->OpacityTables->Create(numActiveLuts);
  this->GradientOpacityTables =
    vtkSmartPointer<vtkOpenGLVolumeLookupTables<vtkOpenGLVolumeGradientOpacityTable> >::New();
  this->GradientOpacityTables->Create(numActiveLuts);

  this->OpacityTablesMap.clear();
  this->RGBTablesMap.clear();
  this->GradientOpacityTablesMap.clear();

  std::ostringstream idx;
  idx << index;

  this->GradientCacheName = "g_gradients_" + idx.str();

  for (int i = 0; i < numActiveLuts; ++i)
  {
    std::ostringstream comp;
    comp << "[" << i << "]";

    this->OpacityTablesMap[i] = "in_opacityTransferFunc_" + idx.str() + comp.str();
    this->RGBTablesMap[i] = "in_colorTransferFunc_" + idx.str() + comp.str();

    // Unlike color and scalar-op, graident-op is optional (some inputs may
    // or may not have gradient-op active).
    if (this->Volume->GetProperty()->HasGradientOpacity())
    {
      this->GradientOpacityTablesMap[i] = "in_gradientTransferFunc_" + idx.str() + comp.str();
    }
  }

  this->LutInit.Modified();
}

void vtkVolumeInputHelper::CreateTransferFunction2D(vtkRenderer* ren, const int index)
{
  this->ReleaseGraphicsTransfer2D(ren->GetRenderWindow());

  unsigned int const num =
    this->ComponentMode == INDEPENDENT ? Texture->GetLoadedScalars()->GetNumberOfComponents() : 1;

  this->TransferFunctions2D =
    vtkSmartPointer<vtkOpenGLVolumeLookupTables<vtkOpenGLVolumeTransferFunction2D> >::New();
  this->TransferFunctions2D->Create(num);

  std::ostringstream idx;
  idx << index;

  this->GradientCacheName = "g_gradients_" + idx.str();

  for (unsigned int i = 0; i < num; i++)
  {
    std::ostringstream comp;
    comp << "[" << i << "]";

    this->TransferFunctions2DMap[i] = "in_transfer2D_" + idx.str() + comp.str();
  }

  this->LutInit.Modified();
}

void vtkVolumeInputHelper::ReleaseGraphicsResources(vtkWindow* window)
{
  this->ReleaseGraphicsTransfer1D(window);
  this->ReleaseGraphicsTransfer2D(window);
  this->Texture->ReleaseGraphicsResources(window);
  this->InitializeTransfer = true;
}

void vtkVolumeInputHelper::ReleaseGraphicsTransfer1D(vtkWindow* window)
{
  if (this->RGBTables)
  {
    this->RGBTables->ReleaseGraphicsResources(window);
  }
  this->RGBTables = nullptr;

  if (this->OpacityTables)
  {
    this->OpacityTables->ReleaseGraphicsResources(window);
  }
  this->OpacityTables = nullptr;

  if (this->GradientOpacityTables)
  {
    this->GradientOpacityTables->ReleaseGraphicsResources(window);
  }
  this->GradientOpacityTables = nullptr;
}

void vtkVolumeInputHelper::ReleaseGraphicsTransfer2D(vtkWindow* window)
{
  if (this->TransferFunctions2D)
  {
    this->TransferFunctions2D->ReleaseGraphicsResources(window);
  }
  this->TransferFunctions2D = nullptr;
}

void vtkVolumeInputHelper::ForceTransferInit()
{
  this->InitializeTransfer = true;
}
