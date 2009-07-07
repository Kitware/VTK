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

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkXMLPHierarchicalBoxDataWriter);
vtkCxxRevisionMacro(vtkXMLPHierarchicalBoxDataWriter, "1.3");

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
  if(this->Controller->GetLocalProcessId() == 0)
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
  vtkCompositeDataSet* hdInput)
{
  this->Superclass::FillDataTypes(hdInput);

  if (!this->Controller)
    {
    return;
    }

  int myid = this->Controller->GetLocalProcessId();
  int numProcs = this->Controller->GetNumberOfProcesses();

  unsigned numBlocks = this->GetNumberOfDataTypes();
  int* myDataTypes = this->GetDataTypesPointer();

  if (myid == 0)
    {
    int* dataTypes = new int[numBlocks];
    for (int i=1; i<numProcs; i++)
      {
      this->Controller->Receive(
        dataTypes, 
        numBlocks, 
        i, 
        vtkMultiProcessController::XML_WRITER_DATA_INFO);
      for (unsigned int j=0; j<numBlocks; j++)
        {
        if (dataTypes[j] >= 0)
          {
          myDataTypes[j] = dataTypes[j];
          }
        }
      }
    delete[] dataTypes;
    }
  else
    {
    this->Controller->Send(myDataTypes, 
                           numBlocks, 
                           0, 
                           vtkMultiProcessController::XML_WRITER_DATA_INFO);
    }

}

