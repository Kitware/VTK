// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
#include "vtkVertexDegree.h"

#include "vtkCommand.h"
#include "vtkGraph.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkVertexDegree);

vtkVertexDegree::vtkVertexDegree()
{
  this->OutputArrayName = nullptr;
}

vtkVertexDegree::~vtkVertexDegree()
{
  // release mem
  this->SetOutputArrayName(nullptr);
}

int vtkVertexDegree::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{

  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkGraph* input = vtkGraph::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkGraph* output = vtkGraph::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

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
  for (int i = 0; i < DegreeArray->GetNumberOfTuples(); ++i)
  {
    DegreeArray->SetValue(i, output->GetDegree(i));

    double progress =
      static_cast<double>(i) / static_cast<double>(DegreeArray->GetNumberOfTuples());
    this->InvokeEvent(vtkCommand::ProgressEvent, &progress);
  }

  // Add attribute array to the output
  output->GetVertexData()->AddArray(DegreeArray);
  DegreeArray->Delete();

  return 1;
}

void vtkVertexDegree::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "OutputArrayName: " << (this->OutputArrayName ? this->OutputArrayName : "(none)")
     << endl;
}
VTK_ABI_NAMESPACE_END
