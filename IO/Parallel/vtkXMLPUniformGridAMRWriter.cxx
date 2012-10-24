/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPUniformGridAMRWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPUniformGridAMRWriter.h"

#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "assert.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkXMLPUniformGridAMRWriter);

vtkCxxSetObjectMacro(vtkXMLPUniformGridAMRWriter,
                     Controller,
                     vtkMultiProcessController);


//----------------------------------------------------------------------------
vtkXMLPUniformGridAMRWriter::vtkXMLPUniformGridAMRWriter()
{
  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());

  // this should be called after the controller is set.
  this->SetWriteMetaFile(1);
}

//----------------------------------------------------------------------------
vtkXMLPUniformGridAMRWriter::~vtkXMLPUniformGridAMRWriter()
{
  this->SetController(0);
}

//----------------------------------------------------------------------------
void vtkXMLPUniformGridAMRWriter::PrintSelf(ostream& os, vtkIndent indent)
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
void vtkXMLPUniformGridAMRWriter::SetWriteMetaFile(int flag)
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
void vtkXMLPUniformGridAMRWriter::FillDataTypes(
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

  if (myid == 0)
    {
    // Collect information about data-types from all satellites and the
    // "combine" the information. Only the root-node needs to have the
    // combined information, since only the root-node writes the XML.
    int *gathered_data_types = new int [numLeafNodes*numProcs];
    for (unsigned int cc=0; cc < numProcs*numLeafNodes; cc++)
      {
      gathered_data_types[cc] = -1;
      }
    this->Controller->Gather(myDataTypes,
      gathered_data_types, numLeafNodes, 0);

    for (int procNo=1; procNo<numProcs; procNo++)
      {
      for (unsigned int pieceNo=0; pieceNo<numLeafNodes; pieceNo++)
        {
        if (myDataTypes[pieceNo] == -1 &&
          gathered_data_types[procNo*numLeafNodes+pieceNo] >= 0)
          {
          myDataTypes[pieceNo] =
            gathered_data_types[procNo*numLeafNodes + pieceNo];
          }
        }
      }
    delete[] gathered_data_types;
    }
  else
    {
    this->Controller->Gather(myDataTypes, NULL, numLeafNodes, 0);
    }
}
