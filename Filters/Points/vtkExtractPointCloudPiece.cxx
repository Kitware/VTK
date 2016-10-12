/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractPointCloudPiece.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractPointCloudPiece.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
// #include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkDoubleArray.h"

vtkStandardNewMacro(vtkExtractPointCloudPiece);

//-----------------------------------------------------------------------------
vtkExtractPointCloudPiece::vtkExtractPointCloudPiece()
{
  this->ModuloOrdering = true;
}

//-----------------------------------------------------------------------------
int vtkExtractPointCloudPiece::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *vtkNotUsed(outputVector))
{
  // get the info object
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);

  return 1;
}

//-----------------------------------------------------------------------------
int vtkExtractPointCloudPiece::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPolyData *input = vtkPolyData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // handle field data
  vtkFieldData *fd = input->GetFieldData();
  vtkFieldData *outFD = output->GetFieldData();
  vtkDataArray *offsets = fd->GetArray("BinOffsets");
  // we wipe the field data as the early viesion of the binner
  // was producing some huge field data that was killing file IO times
  outFD->Initialize();

  // Pipeline update piece will tell us what to generate.
  int piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());

  vtkIdType startIndex, endIndex;
  if (vtkIntArray::SafeDownCast(offsets))
  {
    vtkIntArray *ioffs = vtkIntArray::SafeDownCast(offsets);
    startIndex = ioffs->GetValue(piece);
    endIndex = ioffs->GetValue(piece+1);
  }
  else
  {
    vtkIdTypeArray *ioffs = vtkIdTypeArray::SafeDownCast(offsets);
    startIndex = ioffs->GetValue(piece);
    endIndex = ioffs->GetValue(piece+1);
  }

  vtkIdType numPts = endIndex - startIndex;
  vtkPointData *pd = input->GetPointData();
  vtkPointData *outPD = output->GetPointData();
  outPD->CopyAllocate(pd,numPts);

  vtkNew<vtkPoints> newPoints;
  newPoints->Allocate(numPts);

  newPoints->SetNumberOfPoints(numPts);

  if (this->ModuloOrdering)
  {
    // loop over points copying them to the output
    // we do it in an mod 11 approach to add some randomization to the order
    vtkIdType inIdx = 0;
    vtkIdType nextStart = 1;
    for (vtkIdType i = 0; i < numPts; i++)
    {
      newPoints->SetPoint(i,input->GetPoint(inIdx+startIndex));
      outPD->CopyData(pd, inIdx + startIndex, i);
      inIdx += 11;
      if (inIdx >= numPts)
      {
        inIdx = nextStart;
        nextStart++;
      }
    }
  }
  else // otherwise no reordering
  {
    // copy the points
    newPoints->InsertPoints(0, numPts, startIndex, input->GetPoints());
    // copy point data
    outPD->CopyData(pd, 0, numPts, startIndex);
  }

  output->SetPoints(newPoints.Get());

  return 1;
}

//-----------------------------------------------------------------------------
void vtkExtractPointCloudPiece::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "ModuloOrdering: " << this->ModuloOrdering << "\n";
}
