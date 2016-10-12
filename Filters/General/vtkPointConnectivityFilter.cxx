/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointConnectivityFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPointConnectivityFilter.h"

#include "vtkIdList.h"
#include "vtkUnsignedIntArray.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkSMPTools.h"
#include "vtkSMPThreadLocalObject.h"

#include "vtkNew.h"
#include "vtkSmartPointer.h"

vtkStandardNewMacro(vtkPointConnectivityFilter);

//----------------------------------------------------------------------------
vtkPointConnectivityFilter::vtkPointConnectivityFilter()
{
}

//----------------------------------------------------------------------------
vtkPointConnectivityFilter::~vtkPointConnectivityFilter()
{
}

//----------------------------------------------------------------------------
namespace {

// This class is general purpose for all dataset types.
struct UpdateConnectivityCount
{
  vtkDataSet *Input;
  unsigned int *ConnCount;
  vtkSMPThreadLocalObject<vtkIdList> CellIds;

  UpdateConnectivityCount(vtkDataSet *input, unsigned int *connPtr) :
    Input(input), ConnCount(connPtr)
  {
  }

  void Initialize()
  {
    vtkIdList*& cellIds = this->CellIds.Local();
    cellIds->Allocate(128); //allocate some memory
  }

  void operator() (vtkIdType ptId, vtkIdType endPtId)
  {
      vtkIdList*& cellIds = this->CellIds.Local();
      for ( ; ptId < endPtId; ++ptId )
      {
        this->Input->GetPointCells(ptId, cellIds);
        this->ConnCount[ptId] = cellIds->GetNumberOfIds();
      }
  }

  void Reduce()
  {
  }
};

} // end anon namespace

//----------------------------------------------------------------------------
// This is the generic non-optimized method
int vtkPointConnectivityFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkSmartPointer<vtkDataSet> input = vtkDataSet::GetData(inputVector[0]);
  vtkDataSet *output = vtkDataSet::GetData(outputVector);

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  // Check input
  vtkIdType numPts;
  if ( input == NULL || (numPts=input->GetNumberOfPoints()) < 1 )
  {
    return 1;
  }

  // Create integral array and populate it
  vtkNew<vtkUnsignedIntArray> connCount;
  connCount->SetNumberOfTuples(numPts);
  connCount->SetName("Point Connectivity Count");
  unsigned int *connPtr = static_cast<unsigned int*>(connCount->GetVoidPointer(0));

  // Loop over all points, retrieving connectivity count
  // The first GetPointCells() primes the pump (builds internal structures, etc.)
  vtkNew<vtkIdList> cellIds;
  input->GetPointCells(0, cellIds.GetPointer());
  UpdateConnectivityCount updateCount(input,connPtr);
  vtkSMPTools::For(0,numPts, updateCount);

  // Pass array to the output
  output->GetPointData()->AddArray(connCount.GetPointer());

  return 1;
}

//----------------------------------------------------------------------------
void vtkPointConnectivityFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
