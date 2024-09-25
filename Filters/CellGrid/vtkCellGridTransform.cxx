// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellGridTransform.h"

#include "vtkAbstractTransform.h"
#include "vtkCellGrid.h"
#include "vtkDoubleArray.h"
#include "vtkFiltersCellGrid.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLinearTransform.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"

#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkCellGridTransform);
vtkStandardNewMacro(vtkCellGridTransform::Query);
vtkCxxSetObjectMacro(vtkCellGridTransform::Query, CellAttribute, vtkCellAttribute);
vtkCxxSetObjectMacro(vtkCellGridTransform::Query, Transform, vtkAbstractTransform);

vtkCellGridTransform::Query::Query() = default;

vtkCellGridTransform::Query::~Query()
{
  this->SetCellAttribute(nullptr);
  this->SetTransform(nullptr);
}

void vtkCellGridTransform::Query::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "CellAttribute: " << this->CellAttribute << "\n";
  os << indent << "Transform: " << this->Transform << "\n";
  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
}

vtkMTimeType vtkCellGridTransform::Query::GetMTime()
{
  vtkMTimeType mTime = this->MTime.GetMTime();
  vtkMTimeType transMTime;

  if (this->Transform)
  {
    transMTime = this->Transform->GetMTime();
    mTime = (transMTime > mTime ? transMTime : mTime);
  }

  return mTime;
}

vtkDataArray* vtkCellGridTransform::Query::CreateNewDataArray(vtkDataArray* input) const
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

vtkCellGridTransform::vtkCellGridTransform()
{
  vtkFiltersCellGrid::RegisterCellsAndResponders();
}

vtkCellGridTransform::~vtkCellGridTransform() = default;

void vtkCellGridTransform::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Request:\n";
  vtkIndent i2 = indent.GetNextIndent();
  this->Request->PrintSelf(os, i2);
}

vtkMTimeType vtkCellGridTransform::GetMTime()
{
  auto super = this->Superclass::GetMTime();
  auto req = this->Request->GetMTime();
  return super > req ? super : req;
}

void vtkCellGridTransform::SetTransform(vtkAbstractTransform* tfm)
{
  this->Request->SetTransform(tfm);
}

void vtkCellGridTransform::SetCellAttribute(vtkCellAttribute* att)
{
  this->Request->SetCellAttribute(att);
}

int vtkCellGridTransform::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkSmartPointer<vtkCellGrid> input = vtkCellGrid::GetData(inputVector[0]);
  vtkCellGrid* output = vtkCellGrid::GetData(outputVector);

  if (!input)
  {
    vtkErrorMacro(<< "Invalid or missing input");
    return 0;
  }

  if (!this->Request->GetTransform())
  {
    vtkErrorMacro(<< "No transform provided.");
    return 0;
  }

  output->ShallowCopy(input);
  if (!output->Query(this->Request))
  {
    vtkErrorMacro("Could not transform input.");
    return 0;
  }

  return 1;
}

VTK_ABI_NAMESPACE_END
