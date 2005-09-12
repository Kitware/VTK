/*=========================================================================

  Program:   ParaView
  Module:    vtkXMLPHierarchicalDataWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPHierarchicalDataWriter.h"

#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkXMLPHierarchicalDataWriter);
vtkCxxRevisionMacro(vtkXMLPHierarchicalDataWriter, "1.1");

vtkCxxSetObjectMacro(vtkXMLPHierarchicalDataWriter, 
                     Controller,
                     vtkMultiProcessController);


//----------------------------------------------------------------------------
vtkXMLPHierarchicalDataWriter::vtkXMLPHierarchicalDataWriter()
{
  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkXMLPHierarchicalDataWriter::~vtkXMLPHierarchicalDataWriter()
{
  this->SetController(0);
}

//----------------------------------------------------------------------------
void vtkXMLPHierarchicalDataWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkXMLPHierarchicalDataWriter::FillDataTypes(vtkHierarchicalDataSet* hdInput)
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
        dataTypes, numBlocks, i, vtkMultiProcessController::XML_WRITER_DATA_INFO);
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

