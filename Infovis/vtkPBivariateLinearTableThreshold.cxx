/*=========================================================================
  
Program:   Visualization Toolkit
Module:    vtkPBivariateLinearTableThreshold.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
#include "vtkPBivariateLinearTableThreshold.h"

#include "vtkDataArrayCollection.h"
#include "vtkDataObject.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkTable.h"

#include <map>

vtkStandardNewMacro(vtkPBivariateLinearTableThreshold);
vtkCxxSetObjectMacro(vtkPBivariateLinearTableThreshold, Controller, vtkMultiProcessController);

vtkPBivariateLinearTableThreshold::vtkPBivariateLinearTableThreshold()
{
  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

vtkPBivariateLinearTableThreshold::~vtkPBivariateLinearTableThreshold()
{
  this->SetController(0);
}

void vtkPBivariateLinearTableThreshold::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Controller: " << this->Controller << endl;
}

int vtkPBivariateLinearTableThreshold::RequestData(vtkInformation* request ,
                                              vtkInformationVector** inputVector,
                                              vtkInformationVector* outputVector)
{
  this->Superclass::RequestData(request,inputVector,outputVector);

  // single process?
  if (!this->Controller || this->Controller->GetNumberOfProcesses() <= 1)
    {
    return 1;
    }

 vtkCommunicator* comm = this->Controller->GetCommunicator();
  if (!comm)
    {
    vtkErrorMacro("Need a communicator.");
    return 0;
    }

  vtkTable* outRowDataTable = vtkTable::GetData( outputVector, OUTPUT_ROW_DATA );

  int numProcesses = this->Controller->GetNumberOfProcesses();

 // 2) gather the selected data together
  // for each column, make a new one and add it to a new table
  vtkSmartPointer<vtkTable> gatheredTable = vtkSmartPointer<vtkTable>::New();
  for (int i=0; i<outRowDataTable->GetNumberOfColumns(); i++)
    {
    vtkAbstractArray* col = vtkAbstractArray::SafeDownCast(outRowDataTable->GetColumn(i));
    if (!col)
      continue;

    vtkIdType myLength = col->GetNumberOfTuples();
    vtkIdType totalLength = 0;
    std::vector<vtkIdType> recvLengths(numProcesses,0);
    std::vector<vtkIdType> recvOffsets(numProcesses,0);

    // gathers all of the array lengths together
    comm->AllGather(&myLength, &recvLengths[0], 1);
    
    // compute the displacements
    vtkIdType typeSize = col->GetDataTypeSize();
    for (int j=0; j<numProcesses; j++)
      {
      recvOffsets[j] = totalLength*typeSize;
      totalLength += recvLengths[j];
      recvLengths[j] *= typeSize;
      }
    
    // communicating this as a byte array :/
    vtkAbstractArray* received = vtkAbstractArray::CreateArray(col->GetDataType());
    received->SetNumberOfTuples(totalLength);
    
    char* sendBuf = (char*) col->GetVoidPointer(0);
    char* recvBuf = (char*) received->GetVoidPointer(0);
    
    comm->AllGatherV(sendBuf, recvBuf, myLength*typeSize, &recvLengths[0], &recvOffsets[0]);

    gatheredTable->AddColumn(received);
    received->Delete();
    }

  outRowDataTable->ShallowCopy(gatheredTable);

  return 1;
}
