/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPTableToStructuredGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPTableToStructuredGrid.h"

#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"
#include "vtkStructuredData.h"


static void CopyStructuredData(vtkDataSetAttributes* out, int outExtent[6],
  vtkDataSetAttributes* in, int inExtent[6])
{
  int indims[3];
  indims[0] = inExtent[1] - inExtent[0]+1;
  indims[1] = inExtent[3] - inExtent[2]+1;
  indims[2] = inExtent[5] - inExtent[4]+1;

  int outdims[3];
  outdims[0] = outExtent[1] - outExtent[0]+1;
  outdims[1] = outExtent[3] - outExtent[2]+1;
  outdims[2] = outExtent[5] - outExtent[4]+1;


  int relativeExtent[6];
  relativeExtent[0] = outExtent[0] - inExtent[0];
  relativeExtent[1] = outExtent[1] - inExtent[0];
  relativeExtent[2] = outExtent[2] - inExtent[2];
  relativeExtent[3] = outExtent[3] - inExtent[2];
  relativeExtent[4] = outExtent[4] - inExtent[4];
  relativeExtent[5] = outExtent[5] - inExtent[4];

  for (int zz=relativeExtent[4]; zz <= relativeExtent[5]; zz++)
    {
    for (int yy=relativeExtent[2]; yy <= relativeExtent[3]; yy++)
      {
      for (int xx=relativeExtent[0]; xx <= relativeExtent[1]; xx++)
        {
        int index[3] = {xx, yy, zz};
        int outindex[3] = {xx-relativeExtent[0], yy-relativeExtent[2],
          zz-relativeExtent[4]};
        out->CopyData(in,
          vtkStructuredData::ComputePointId(indims, index),
          vtkStructuredData::ComputePointId(outdims, outindex));
        }
      }
    }
  
}


vtkStandardNewMacro(vtkPTableToStructuredGrid);
vtkCxxSetObjectMacro(vtkPTableToStructuredGrid, Controller,
  vtkMultiProcessController);
//----------------------------------------------------------------------------
vtkPTableToStructuredGrid::vtkPTableToStructuredGrid()
{
  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkPTableToStructuredGrid::~vtkPTableToStructuredGrid()
{
  this->SetController(0);
}

//----------------------------------------------------------------------------
int vtkPTableToStructuredGrid::RequestData(vtkInformation* request,
    vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  int numProcs = this->Controller->GetNumberOfProcesses();
  int myId = this->Controller->GetLocalProcessId();

  if (numProcs <= 1)
    {
    return this->Superclass::RequestData(request, inputVector, outputVector);
    }

  vtkStructuredGrid* output = vtkStructuredGrid::GetData(outputVector, 0);
  vtkTable* input = vtkTable::GetData(inputVector[0], 0);

  int data_valid = 0;
  if (myId == 0)
    {
    // Ensure that extents are valid.
    int num_values = (this->WholeExtent[1] - this->WholeExtent[0] + 1) *
      (this->WholeExtent[3] - this->WholeExtent[2] + 1) *
      (this->WholeExtent[5] - this->WholeExtent[4] + 1);

    if (input->GetNumberOfRows() != num_values)
      {
      vtkErrorMacro("The input table must have exactly " << num_values
        << " rows. Currently it has " << input->GetNumberOfRows() << " rows.");
      }
    else
      {
      data_valid = 1;
      }
    }
  this->Controller->Broadcast(&data_valid, 1, 0);
  if (!data_valid)
    {
    return 0;
    }

  vtkStreamingDemandDrivenPipeline *sddp = 
    vtkStreamingDemandDrivenPipeline::SafeDownCast(this->GetExecutive());
  int extent[6];
  sddp->GetOutputInformation(0)->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), extent);

  int *allextents = new int[numProcs*6];
  if (!this->Controller->Gather(extent, allextents, 6, 0))
    {
    vtkErrorMacro("Failed to gather extents.");
    return 0;
    }
  if (myId == 0)
    {
    // Send the relevant rows to each of the processes.
    for (int cc=0; cc < numProcs; cc++)
      {
      int curExtent[6];
      memcpy(curExtent, &allextents[6*cc], 6*sizeof(int));
      vtkIdType numTuples = (curExtent[1]-curExtent[0] + 1) * 
        (curExtent[3]-curExtent[2] + 1) * 
        (curExtent[5]-curExtent[4] + 1); 

      vtkTable* curTable = vtkTable::New();
      curTable->GetRowData()->CopyAllocate(input->GetRowData(), numTuples);
      ::CopyStructuredData(curTable->GetRowData(), curExtent,
        input->GetRowData(), this->WholeExtent);
      if (cc==0)
        {
        this->Superclass::Convert(curTable, output, curExtent);
        }
      else
        {
        this->Controller->Send(curTable, cc, 985723);
        }
      curTable->Delete();
      }
    }
  else
    {
    vtkTable* curTable = vtkTable::New();
    this->Controller->Receive(curTable, 0, 985723);
    this->Superclass::Convert(curTable, output, extent);
    curTable->Delete();
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkPTableToStructuredGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}

