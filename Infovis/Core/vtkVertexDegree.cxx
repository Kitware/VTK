/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVertexDegree.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
#include "vtkVertexDegree.h"

#include "vtkCommand.h"
#include "vtkGraph.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

vtkStandardNewMacro(vtkVertexDegree);

vtkVertexDegree::vtkVertexDegree()
{
  this->OutputArrayName = 0;
}

vtkVertexDegree::~vtkVertexDegree()
{
  // release mem
  this->SetOutputArrayName(0);
}


int vtkVertexDegree::RequestData(vtkInformation *vtkNotUsed(request),
                            vtkInformationVector **inputVector,
                            vtkInformationVector *outputVector)
{

  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkGraph *input = vtkGraph::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkGraph *output = vtkGraph::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Do a shallow copy of the input to the output
  output->ShallowCopy(input);

  // Create the attribute array
  vtkIntArray* DegreeArray = vtkIntArray::New();
  if (this->OutputArrayName)
  {
    DegreeArray->SetName(this->OutputArrayName);
  }
  else
  {
    DegreeArray->SetName("VertexDegree");
  }
  DegreeArray->SetNumberOfTuples(output->GetNumberOfVertices());

  // Now loop through the vertices and set their degree in the array
  for(int i=0;i< DegreeArray->GetNumberOfTuples(); ++i)
  {
    DegreeArray->SetValue(i,output->GetDegree(i));

    double progress = static_cast<double>(i) / static_cast<double>(DegreeArray->GetNumberOfTuples());
    this->InvokeEvent(vtkCommand::ProgressEvent, &progress);
  }

  // Add attribute array to the output
  output->GetVertexData()->AddArray(DegreeArray);
  DegreeArray->Delete();

  return 1;
}

void vtkVertexDegree::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "OutputArrayName: "
     << (this->OutputArrayName ? this->OutputArrayName : "(none)") << endl;

}
