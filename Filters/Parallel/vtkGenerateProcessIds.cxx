// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkGenerateProcessIds.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkPointData.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkGenerateProcessIds);

vtkCxxSetSmartPointerMacro(vtkGenerateProcessIds, Controller, vtkMultiProcessController);

//------------------------------------------------------------------------------
vtkGenerateProcessIds::vtkGenerateProcessIds()
{
  this->Controller = vtkMultiProcessController::GetGlobalController();
}

//------------------------------------------------------------------------------
vtkGenerateProcessIds::~vtkGenerateProcessIds() = default;

//------------------------------------------------------------------------------
int vtkGenerateProcessIds::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (!inInfo || !outInfo)
  {
    return 0;
  }

  // get the input and output
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet* output = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!input || !output)
  {
    return 0;
  }

  vtkIdType piece =
    this->Controller ? static_cast<vtkIdType>(this->Controller->GetLocalProcessId()) : 0;
  output->ShallowCopy(input);

  if (this->GeneratePointData)
  {
    vtkIdType numberOfPoints = input->GetNumberOfPoints();
    vtkSmartPointer<vtkIdTypeArray> processIds = this->GenerateProcessIds(piece, numberOfPoints);
    processIds->SetName("PointProcessIds");
    output->GetPointData()->SetProcessIds(processIds);
  }
  if (this->GenerateCellData)
  {
    vtkIdType numberOfCells = input->GetNumberOfCells();
    vtkSmartPointer<vtkIdTypeArray> processIds = this->GenerateProcessIds(piece, numberOfCells);
    processIds->SetName("CellProcessIds");
    output->GetCellData()->SetProcessIds(processIds);
  }

  return 1;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkIdTypeArray> vtkGenerateProcessIds::GenerateProcessIds(
  vtkIdType piece, vtkIdType numberOfTuples)
{
  vtkSmartPointer<vtkIdTypeArray> processIds = vtkSmartPointer<vtkIdTypeArray>::New();
  processIds->SetNumberOfTuples(numberOfTuples);
  processIds->Fill(piece);
  return processIds;
}

//------------------------------------------------------------------------------
vtkMultiProcessController* vtkGenerateProcessIds::GetController()
{
  return this->Controller.Get();
}

//------------------------------------------------------------------------------
void vtkGenerateProcessIds::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Generate for PointData: " << (this->GeneratePointData ? "On" : "Off")
     << std::endl;
  os << indent << "Generate for CellData: " << (this->GenerateCellData ? "On" : "Off") << std::endl;

  os << indent << "Controller: ";
  if (this->Controller)
  {
    this->Controller->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)" << std::endl;
  }
}
VTK_ABI_NAMESPACE_END
