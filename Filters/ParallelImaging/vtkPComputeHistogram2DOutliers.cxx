// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2009 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
#include "vtkPComputeHistogram2DOutliers.h"
//------------------------------------------------------------------------------
#include "vtkCollection.h"
#include "vtkCommunicator.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkImageGradientMagnitude.h"
#include "vtkImageMedian3D.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"
//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPComputeHistogram2DOutliers);
vtkCxxSetObjectMacro(vtkPComputeHistogram2DOutliers, Controller, vtkMultiProcessController);
//------------------------------------------------------------------------------
vtkPComputeHistogram2DOutliers::vtkPComputeHistogram2DOutliers()
{
  this->Controller = nullptr;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}
//------------------------------------------------------------------------------
vtkPComputeHistogram2DOutliers::~vtkPComputeHistogram2DOutliers()
{
  this->SetController(nullptr);
}
//------------------------------------------------------------------------------
void vtkPComputeHistogram2DOutliers::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}
//------------------------------------------------------------------------------
int vtkPComputeHistogram2DOutliers::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (!this->Superclass::RequestData(request, inputVector, outputVector))
    return 0;

  if (!this->Controller || this->Controller->GetNumberOfProcesses() <= 1)
  {
    // Nothing to do for single process.
    return 1;
  }

  vtkCommunicator* comm = this->Controller->GetCommunicator();
  if (!comm)
  {
    vtkErrorMacro("Need a communicator.");
    return 0;
  }

  // get the output
  vtkInformation* outTableInfo = outputVector->GetInformationObject(OUTPUT_SELECTED_TABLE_DATA);
  vtkTable* outputTable = vtkTable::SafeDownCast(outTableInfo->Get(vtkDataObject::DATA_OBJECT()));

  int numProcesses = this->Controller->GetNumberOfProcesses();
  // int myId = this->Controller->GetLocalProcessId();

  // 1) leave the selected rows alone, since they don't make sense for multiple nodes
  //

  // 2) gather the selected data together
  // for each column, make a new one and add it to a new table
  vtkSmartPointer<vtkTable> gatheredTable = vtkSmartPointer<vtkTable>::New();
  for (int i = 0; i < outputTable->GetNumberOfColumns(); i++)
  {
    vtkAbstractArray* col = vtkArrayDownCast<vtkAbstractArray>(outputTable->GetColumn(i));
    if (!col)
      continue;

    vtkIdType myLength = col->GetNumberOfTuples();
    vtkIdType totalLength = 0;
    std::vector<vtkIdType> recvLengths(numProcesses, 0);
    std::vector<vtkIdType> recvOffsets(numProcesses, 0);

    // gathers all of the array lengths together
    comm->AllGather(&myLength, recvLengths.data(), 1);

    // compute the displacements
    vtkIdType typeSize = col->GetDataTypeSize();
    for (int j = 0; j < numProcesses; j++)
    {
      recvOffsets[j] = totalLength * typeSize;
      totalLength += recvLengths[j];
      recvLengths[j] *= typeSize;
    }

    // communicating this as a byte array :/
    vtkAbstractArray* received = vtkAbstractArray::CreateArray(col->GetDataType());
    received->SetNumberOfTuples(totalLength);

    char* sendBuf = (char*)col->GetVoidPointer(0);
    char* recvBuf = (char*)received->GetVoidPointer(0);

    comm->AllGatherV(sendBuf, recvBuf, myLength * typeSize, recvLengths.data(), recvOffsets.data());

    gatheredTable->AddColumn(received);
    received->Delete();
  }

  outputTable->ShallowCopy(gatheredTable);

  return 1;
}
VTK_ABI_NAMESPACE_END
