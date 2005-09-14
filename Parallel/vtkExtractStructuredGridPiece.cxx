/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractStructuredGridPiece.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractStructuredGridPiece.h"

#include "vtkCell.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedCharArray.h"
#include "vtkStructuredGrid.h"
#include "vtkDoubleArray.h"

vtkCxxRevisionMacro(vtkExtractStructuredGridPiece, "1.4");
vtkStandardNewMacro(vtkExtractStructuredGridPiece);

int vtkExtractStructuredGridPiece::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *vtkNotUsed(outputVector))
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
              inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 
              6);
  return 1;
}
  
int vtkExtractStructuredGridPiece::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  vtkStructuredGrid *input = vtkStructuredGrid::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkStructuredGrid *output = vtkStructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Decide what to extract.
  int uExtent[6] = {0,-1,0,-1,0,-1};
  int wExtent[6] = {0,-1,0,-1,0,-1};

  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), uExtent);
  outInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wExtent);

  // Extract structure.
  output->SetDimensions(input->GetDimensions());
  output->SetExtent(uExtent);

  int wsizej = wExtent[3]-wExtent[2]+1;
  int wsizei = wExtent[1]-wExtent[0]+1;

  int usizek = uExtent[5]-uExtent[4]+1;
  int usizej = uExtent[3]-uExtent[2]+1;
  int usizei = uExtent[1]-uExtent[0]+1;
  int usize  = usizek*usizej*usizei; 

  vtkPoints *ip = input->GetPoints();  
  vtkPoints *op = vtkPoints::New();
  op->SetNumberOfPoints(usize);

  double coords[3];
  vtkIdType pCtr = 0;
  for (int k = uExtent[4]; k <= uExtent[5]; k++) 
    {
    for (int j = uExtent[2]; j <= uExtent[3]; j++) 
      {
      for (int i = uExtent[0]; i <= uExtent[1]; i++) 
        {
        vtkIdType pointId = k*wsizej*wsizei +  j*wsizei + i;
        ip->GetPoint(pointId, coords);
        op->SetPoint(pCtr++, coords);
        }
      }        
    }
  op->Squeeze();
  output->SetPoints(op);
  op->Delete();

  // Extract attributes.
  vtkPointData *ipd = input->GetPointData();  
  vtkPointData *opd = output->GetPointData();
  opd->CopyAllocate(ipd, usize, 1000);

  vtkCellData *icd = input->GetCellData();
  vtkCellData *ocd = output->GetCellData();
  ocd->CopyAllocate(icd, usize, 1000);

  vtkIdType ptCtr = 0;
  vtkIdType clCtr = 0;
  for (int k = uExtent[4]; k <= uExtent[5]; k++) 
    {
    for (int j = uExtent[2]; j <= uExtent[3]; j++) 
      {
      for (int i = uExtent[0]; i <= uExtent[1]; i++) 
        {
        vtkIdType pointId = k*wsizej*wsizei +  j*wsizei + i;
        opd->CopyData(ipd, pointId, ptCtr++);
        if ((k != uExtent[5]) && (j != uExtent[3]) && (i != uExtent[1]))
          {
          vtkIdType cellId = k*(wsizej-1)*(wsizei-1) +  j*(wsizei-1) + i;
          ocd->CopyData(icd, cellId, clCtr++);
          }
        }
      }        
    }
  opd->Squeeze();
  ocd->Squeeze();

  //get field data
  vtkFieldData *newFieldData = vtkFieldData::New();
  newFieldData->DeepCopy(input->GetFieldData());
  output->SetFieldData(newFieldData);
  newFieldData->Delete();

  return 1;
}

void vtkExtractStructuredGridPiece::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}


