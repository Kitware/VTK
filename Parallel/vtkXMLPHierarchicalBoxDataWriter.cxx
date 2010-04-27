/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPHierarchicalBoxDataWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPHierarchicalBoxDataWriter.h"

#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "assert.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkXMLPHierarchicalBoxDataWriter);

vtkCxxSetObjectMacro(vtkXMLPHierarchicalBoxDataWriter, 
                     Controller,
                     vtkMultiProcessController);


//----------------------------------------------------------------------------
vtkXMLPHierarchicalBoxDataWriter::vtkXMLPHierarchicalBoxDataWriter()
{
  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());
  this->vtkXMLPHierarchicalBoxDataWriter::SetWriteMetaFile(1);
}

//----------------------------------------------------------------------------
vtkXMLPHierarchicalBoxDataWriter::~vtkXMLPHierarchicalBoxDataWriter()
{
  this->SetController(0);
}

//----------------------------------------------------------------------------
void vtkXMLPHierarchicalBoxDataWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Controller: ";
  if (this->Controller)
    {
    this->Controller->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
}

//----------------------------------------------------------------------------
void vtkXMLPHierarchicalBoxDataWriter::SetWriteMetaFile(int flag)
{
  this->Modified();
  if(this->Controller == NULL || this->Controller->GetLocalProcessId() == 0)
    {
    if(this->WriteMetaFile != flag)
      {
      this->WriteMetaFile = flag;
      }
    }
  else
    {
    this->WriteMetaFile = 0;
    }
}

//----------------------------------------------------------------------------
void vtkXMLPHierarchicalBoxDataWriter::FillDataTypes(
  vtkCompositeDataSet* cdInput)
{
  this->Superclass::FillDataTypes(cdInput);

  if (!this->Controller )
    {
    return;
    }

  int myid = this->Controller->GetLocalProcessId();
  int numProcs = this->Controller->GetNumberOfProcesses();
  unsigned int numLeafNodes = this->GetNumberOfDataTypes();
  int* myDataTypes = this->GetDataTypesPointer();
  if (numProcs == 1 || numLeafNodes == 0)
    {
    return;
    }


  // Collect information about amr-boxes (we don't need to gather refinement
  // ratio informations since those are certainly consistent on all processes
  // since we expect the same composite structure on all nodes.
  assert(this->AMRBoxes != NULL);
  assert(this->AMRBoxDims != NULL);

  if (myid == 0)
    {
    int *gathered_data_types = new int [numLeafNodes*numProcs];
    for (unsigned int cc=0; cc < numProcs*numLeafNodes; cc++)
      {
      gathered_data_types[cc] = -1;
      }
    this->Controller->Gather(myDataTypes,
      gathered_data_types, numLeafNodes, 0);

    int *gathered_amx_box_dims = new int [numLeafNodes*6*numProcs];
    memset(gathered_amx_box_dims, 0, numLeafNodes*6*numProcs*sizeof(int));
    this->Controller->Gather(this->AMRBoxes, gathered_amx_box_dims,
      numLeafNodes*6, 0);
    this->Controller->Gather(this->AMRBoxDims, gathered_amx_box_dims,
      numLeafNodes, 0);

    for (int procNo=1; procNo<numProcs; procNo++)
      {
      for (unsigned int pieceNo=0; pieceNo<numLeafNodes; pieceNo++)
        {
        if (myDataTypes[pieceNo] == -1 &&
          gathered_data_types[procNo*numLeafNodes+pieceNo] >= 0)
          {
          myDataTypes[pieceNo] =
            gathered_data_types[procNo*numLeafNodes + pieceNo];
          memcpy(&this->AMRBoxes[pieceNo*6],
            &gathered_amx_box_dims[(procNo*numLeafNodes + pieceNo)*6],
            sizeof(int)*6);
          this->AMRBoxDims[pieceNo] = gathered_amx_box_dims[
            procNo*numLeafNodes + pieceNo];
          }
        }
      }
    delete[] gathered_data_types;
    delete[] gathered_amx_box_dims;
    }
  else
    {
    this->Controller->Gather(myDataTypes, NULL, numLeafNodes, 0);
    this->Controller->Gather(this->AMRBoxes, NULL, numLeafNodes*6, 0);
    this->Controller->Gather(this->AMRBoxDims, NULL, numLeafNodes, 0);
    }
}

