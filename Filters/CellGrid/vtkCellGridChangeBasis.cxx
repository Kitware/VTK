// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellGridChangeBasis.h"

#include "vtkCellGrid.h"
#include "vtkDoubleArray.h"
#include "vtkFiltersCellGrid.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"

#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkCellGridChangeBasis);
vtkStandardNewMacro(vtkCellGridChangeBasis::Query);
vtkCxxSetObjectMacro(vtkCellGridChangeBasis::Query, CellAttribute, vtkCellAttribute);

vtkCellGridChangeBasis::Query::Query() = default;

vtkCellGridChangeBasis::Query::~Query()
{
  this->SetCellAttribute(nullptr);
}

void vtkCellGridChangeBasis::Query::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "CellAttribute: " << this->CellAttribute << "\n";
  os << indent << "SourceFunctionSpace: " << this->SourceFunctionSpace.Data() << "\n";
  os << indent << "SourceBasis: " << this->SourceBasis.Data() << "\n";
  os << indent << "SourceOrder: " << this->SourceOrder << "\n";
  os << indent << "TargetFunctionSpace: " << this->TargetFunctionSpace.Data() << "\n";
  os << indent << "TargetBasis: " << this->TargetBasis.Data() << "\n";
  os << indent << "TargetOrder: " << this->TargetOrder << "\n";
  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
}

vtkMTimeType vtkCellGridChangeBasis::Query::GetMTime()
{
  vtkMTimeType mTime = this->MTime.GetMTime();
  vtkMTimeType attribMTime;

  if (this->CellAttribute)
  {
    attribMTime = this->CellAttribute->GetMTime();
    mTime = (attribMTime > mTime ? attribMTime : mTime);
  }

  return mTime;
}

bool vtkCellGridChangeBasis::Query::Initialize()
{
  bool result = this->Superclass::Initialize();
  this->SharedOutputs.clear();
  return result;
}

vtkDataArray* vtkCellGridChangeBasis::Query::CreateNewDataArray(vtkDataArray* input) const
{
  if (this->OutputPointsPrecision == vtkAlgorithm::DEFAULT_PRECISION && input != nullptr)
  {
    return input->NewInstance();
  }

  switch (this->OutputPointsPrecision)
  {
    case vtkAlgorithm::DOUBLE_PRECISION:
      return vtkDoubleArray::New();
    case vtkAlgorithm::SINGLE_PRECISION:
    default:
      return vtkFloatArray::New();
  }
}

// ------------

vtkCellGridChangeBasis::vtkCellGridChangeBasis()
{
  vtkFiltersCellGrid::RegisterCellsAndResponders();
}

vtkCellGridChangeBasis::~vtkCellGridChangeBasis() = default;

void vtkCellGridChangeBasis::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Request:\n";
  vtkIndent i2 = indent.GetNextIndent();
  this->Request->PrintSelf(os, i2);
}

vtkMTimeType vtkCellGridChangeBasis::GetMTime()
{
  return std::max(this->Superclass::GetMTime(), this->Request->GetMTime());
}

void vtkCellGridChangeBasis::SetCellAttribute(vtkCellAttribute* att)
{
  this->Request->SetCellAttribute(att);
}

void vtkCellGridChangeBasis::SetSourceFunctionSpace(const char* functionSpace)
{
  vtkStringToken fs;
  if (functionSpace && functionSpace[0])
  {
    fs = vtkStringToken(functionSpace);
  }
  this->Request->SetSourceFunctionSpace(fs);
}

void vtkCellGridChangeBasis::SetSourceBasis(const char* basis)
{
  vtkStringToken bs;
  if (basis && basis[0])
  {
    bs = vtkStringToken(basis);
  }
  this->Request->SetSourceBasis(bs);
}

void vtkCellGridChangeBasis::SetSourceOrder(int order)
{
  this->Request->SetSourceOrder(order);
}

void vtkCellGridChangeBasis::SetTargetFunctionSpace(const char* functionSpace)
{
  vtkStringToken fs;
  if (functionSpace && functionSpace[0])
  {
    fs = vtkStringToken(functionSpace);
  }
  this->Request->SetTargetFunctionSpace(fs);
}

void vtkCellGridChangeBasis::SetTargetBasis(const char* basis)
{
  vtkStringToken bs;
  if (basis && basis[0])
  {
    bs = vtkStringToken(basis);
  }
  this->Request->SetTargetBasis(bs);
}

void vtkCellGridChangeBasis::SetTargetOrder(int order)
{
  this->Request->SetTargetOrder(order);
}

int vtkCellGridChangeBasis::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkCellGrid* input = vtkCellGrid::GetData(inputVector[0]);
  vtkCellGrid* output = vtkCellGrid::GetData(outputVector);

  if (!input)
  {
    vtkErrorMacro(<< "Invalid or missing input");
    return 0;
  }

  output->ShallowCopy(input);
  if (!output->Query(this->Request))
  {
    vtkErrorMacro("Could not convert input.");
    return 0;
  }

  return 1;
}

VTK_ABI_NAMESPACE_END
