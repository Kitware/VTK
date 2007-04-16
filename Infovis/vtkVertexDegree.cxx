/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVertexDegree.cxx

  Copyright 2007 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

=========================================================================*/

#include "vtkVertexDegree.h"

#include "vtkAbstractGraph.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

vtkCxxRevisionMacro(vtkVertexDegree, "1.1");
vtkStandardNewMacro(vtkVertexDegree);

vtkVertexDegree::vtkVertexDegree()
{
  this->OutputArrayName = 0;
}

vtkVertexDegree::~vtkVertexDegree()
{
}


int vtkVertexDegree::RequestData(vtkInformation *vtkNotUsed(request),
                            vtkInformationVector **inputVector,
                            vtkInformationVector *outputVector)
{

  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkAbstractGraph *input = vtkAbstractGraph::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkAbstractGraph *output = vtkAbstractGraph::SafeDownCast(
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
  
  // Now loop through the verticies and set their degree in the array
  for(int i=0;i< DegreeArray->GetNumberOfTuples(); ++i)
    {
    DegreeArray->SetValue(i,output->GetDegree(i));
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
