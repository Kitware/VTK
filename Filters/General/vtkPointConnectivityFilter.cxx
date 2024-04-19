// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPointConnectivityFilter.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkUnsignedIntArray.h"

#include "vtkNew.h"
#include "vtkSmartPointer.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPointConnectivityFilter);

//------------------------------------------------------------------------------
vtkPointConnectivityFilter::vtkPointConnectivityFilter() = default;

//------------------------------------------------------------------------------
vtkPointConnectivityFilter::~vtkPointConnectivityFilter() = default;

//------------------------------------------------------------------------------
namespace
{

// This class is general purpose for all dataset types.
struct UpdateConnectivityCount
{
  vtkDataSet* Input;
  unsigned int* ConnCount;
  vtkSMPThreadLocalObject<vtkIdList> CellIds;
  vtkPointConnectivityFilter* Filter;

  UpdateConnectivityCount(
    vtkDataSet* input, unsigned int* connPtr, vtkPointConnectivityFilter* filter)
    : Input(input)
    , ConnCount(connPtr)
    , Filter(filter)
  {
  }

  void Initialize()
  {
    vtkIdList*& cellIds = this->CellIds.Local();
    cellIds->Allocate(128); // allocate some memory
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    vtkIdList*& cellIds = this->CellIds.Local();
    bool isFirst = vtkSMPTools::GetSingleThread();
    for (; ptId < endPtId; ++ptId)
    {
      if (isFirst)
      {
        this->Filter->CheckAbort();
      }
      if (this->Filter->GetAbortOutput())
      {
        break;
      }
      this->Input->GetPointCells(ptId, cellIds);
      this->ConnCount[ptId] = cellIds->GetNumberOfIds();
    }
  }

  void Reduce() {}
};

} // end anon namespace

//------------------------------------------------------------------------------
// This is the generic non-optimized method
int vtkPointConnectivityFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkSmartPointer<vtkDataSet> input = vtkDataSet::GetData(inputVector[0]);
  vtkDataSet* output = vtkDataSet::GetData(outputVector);

  // First, copy the input to the output as a starting point
  output->CopyStructure(input);
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  // Check input
  vtkIdType numPts;
  if (input == nullptr || (numPts = input->GetNumberOfPoints()) < 1)
  {
    return 1;
  }

  // Create integral array and populate it
  vtkNew<vtkUnsignedIntArray> connCount;
  connCount->SetNumberOfTuples(numPts);
  connCount->SetName("Point Connectivity Count");
  unsigned int* connPtr = connCount->GetPointer(0);

  // Loop over all points, retrieving connectivity count
  // The first GetPointCells() primes the pump (builds internal structures, etc.)
  vtkNew<vtkIdList> cellIds;
  input->GetPointCells(0, cellIds);
  UpdateConnectivityCount updateCount(input, connPtr, this);
  vtkSMPTools::For(0, numPts, updateCount);

  // Pass array to the output
  output->GetPointData()->AddArray(connCount);

  return 1;
}

//------------------------------------------------------------------------------
void vtkPointConnectivityFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
