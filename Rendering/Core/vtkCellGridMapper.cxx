// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellGridMapper.h"
#include "vtkAbstractMapper.h"
#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkColorTransferFunction.h"
#include "vtkDoubleArray.h"
#include "vtkExecutive.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkLookupTable.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVariant.h"
#include <iostream>

VTK_ABI_NAMESPACE_BEGIN

vtkObjectFactoryNewMacro(vtkCellGridMapper);

vtkCellGridMapper::vtkCellGridMapper() = default;

void vtkCellGridMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "VisualizePCoords: " << this->VisualizePCoords << "\n";
  os << indent << "VisualizeBasisFunction: " << this->VisualizeBasisFunction << "\n";
}

void vtkCellGridMapper::SetInputData(vtkCellGrid* in)
{
  this->SetInputDataInternal(0, in);
}

vtkCellGrid* vtkCellGridMapper::GetInput()
{
  vtkCellGrid* result = nullptr;
  if (auto* exec = this->GetExecutive())
  {
    result = vtkCellGrid::SafeDownCast(exec->GetInputData(0, 0));
  }
  return result;
}

void vtkCellGridMapper::PrepareColormap(vtkScalarsToColors* cmap)
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

// Get the bounds for the input of this mapper as
// (xMin, xMax, yMin, yMax, zMin, zMax)
double* vtkCellGridMapper::GetBounds()
{
  if (!this->GetNumberOfInputConnections(0))
  {
    vtkMath::UninitializeBounds(this->Bounds);
    return this->Bounds;
  }
  else
  {
    if (!this->Static)
    {
      vtkInformation* inInfo = this->GetInputInformation();
      if (inInfo)
      {
        this->GetInputAlgorithm()->UpdateInformation();
        this->GetInputAlgorithm()->Update();
      }
    }
    this->ComputeBounds();

    // if the bounds indicate NAN
    if (!vtkMath::AreBoundsInitialized(this->Bounds))
    {
      return nullptr;
    }
    return this->Bounds;
  }
}

void vtkCellGridMapper::ComputeBounds()
{
  vtkCellGrid* input = this->GetInput();
  if (input && input->GetNumberOfElements(vtkDataObject::CELL))
  {
    input->GetBounds(this->Bounds);
  }
  else
  {
    vtkMath::UninitializeBounds(this->Bounds);
  }
}

int vtkCellGridMapper::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCellGrid");
  return 1;
}

void vtkCellGridMapper::Update(int port)
{
  if (this->Static)
  {
    return;
  }
  this->Superclass::Update(port);
}

void vtkCellGridMapper::Update()
{
  if (this->Static)
  {
    return;
  }
  this->Superclass::Update();
}

vtkTypeBool vtkCellGridMapper::Update(int port, vtkInformationVector* requests)
{
  if (this->Static)
  {
    return 1;
  }
  return this->Superclass::Update(port, requests);
}

vtkTypeBool vtkCellGridMapper::Update(vtkInformation* requests)
{
  if (this->Static)
  {
    return 1;
  }
  return this->Superclass::Update(requests);
}

void vtkCellGridMapper::CreateColormapTexture()
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

vtkCellAttribute* vtkCellGridMapper::GetColorAttribute(vtkCellGrid* input) const
{
  vtkCellAttribute* attribute = nullptr;
  if (input)
  {
    if (this->ArrayAccessMode == VTK_GET_ARRAY_BY_NAME)
    {
      attribute = input->GetCellAttributeByName(this->ArrayName);
    }
    else // if (this->ArrayAccessMode == VTK_GET_ARRAY_BY_ID)
    {
      attribute = input->GetCellAttributeById(this->ArrayId);
    }
  }
  return attribute;
}

bool vtkCellGridMapper::HasTranslucentPolygonalGeometry()
{
  // TODO: We should determine whether coloring by a scalar
  //       and, if so, whether the colormap has any opacity
  //       values in ]0, 1[.
  return false;
}
VTK_ABI_NAMESPACE_END
