/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractVectorComponents.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractVectorComponents.h"

#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkExecutive.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

vtkStandardNewMacro(vtkExtractVectorComponents);

vtkExtractVectorComponents::vtkExtractVectorComponents()
{
  this->ExtractToFieldData = 0;
  this->SetNumberOfOutputPorts(3);
  this->OutputsInitialized = 0;
}

vtkExtractVectorComponents::~vtkExtractVectorComponents()
{
}

// Get the output dataset representing velocity x-component. If output is NULL
// then input hasn't been set, which is necessary for abstract objects. (Note:
// this method returns the same information as the GetOutput() method with an
// index of 0.)
vtkDataSet *vtkExtractVectorComponents::GetVxComponent()
{
  return this->GetOutput(0);
}

// Get the output dataset representing velocity y-component. If output is NULL
// then input hasn't been set, which is necessary for abstract objects. (Note:
// this method returns the same information as the GetOutput() method with an
// index of 1.)
vtkDataSet *vtkExtractVectorComponents::GetVyComponent()
{
  return this->GetOutput(1);
}

// Get the output dataset representing velocity z-component. If output is NULL
// then input hasn't been set, which is necessary for abstract objects. (Note:
// this method returns the same information as the GetOutput() method with an
// index of 2.)
vtkDataSet *vtkExtractVectorComponents::GetVzComponent()
{
  return this->GetOutput(2);
}

// Specify the input data or filter.
void vtkExtractVectorComponents::SetInputData(vtkDataSet *input)
{
  if (this->GetNumberOfInputConnections(0) > 0 && this->GetInput(0) == input )
    {
    return;
    }

  this->Superclass::SetInputData(0, input);

  if ( input == NULL )
    {
    return;
    }

  vtkDataSet *output;
  if ( ! this->OutputsInitialized )
    {
    output = input->NewInstance();
    this->GetExecutive()->SetOutputData(0, output);
    output->Delete();
    output = input->NewInstance();
    this->GetExecutive()->SetOutputData(1, output);
    output->Delete();
    output = input->NewInstance();
    this->GetExecutive()->SetOutputData(2, output);
    output->Delete();
    this->OutputsInitialized = 1;
    return;
    }

  // since the input has changed we might need to create a new output
  // It seems that output 0 is the correct type as a result of the call to
  // the superclass's SetInput.  Check the type of output 1 instead.
  if (strcmp(this->GetOutput(1)->GetClassName(),input->GetClassName()))
    {
    output = input->NewInstance();
    this->GetExecutive()->SetOutputData(0, output);
    output->Delete();
    output = input->NewInstance();
    this->GetExecutive()->SetOutputData(1, output);
    output->Delete();
    output = input->NewInstance();
    this->GetExecutive()->SetOutputData(2, output);
    output->Delete();
    vtkWarningMacro(<<" a new output had to be created since the input type changed.");
    }
}

template <class T>
void vtkExtractComponents(int numVectors, T* vectors, T* vx, T* vy, T* vz)
{
  for (int i=0; i<numVectors; i++)
    {
    vx[i] = vectors[3*i];
    vy[i] = vectors[3*i+1];
    vz[i] = vectors[3*i+2];
    }
}

int vtkExtractVectorComponents::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  int numVectors = 0, numVectorsc = 0;
  vtkDataArray *vectors, *vectorsc;
  vtkDataArray *vx, *vy, *vz;
  vtkDataArray *vxc, *vyc, *vzc;
  vtkPointData *pd, *outVx, *outVy=0, *outVz=0;
  vtkCellData *cd, *outVxc, *outVyc=0, *outVzc=0;

  vtkDebugMacro(<<"Extracting vector components...");

  // taken out of previous update method.
  output->CopyStructure(input);
  if (!this->ExtractToFieldData)
    {
    this->GetVyComponent()->CopyStructure(input);
    this->GetVzComponent()->CopyStructure(input);
    }

  pd = input->GetPointData();
  cd = input->GetCellData();
  outVx = output->GetPointData();
  outVxc = output->GetCellData();
  if (!this->ExtractToFieldData)
    {
    outVy = this->GetVyComponent()->GetPointData();
    outVz = this->GetVzComponent()->GetPointData();
    outVyc = this->GetVyComponent()->GetCellData();
    outVzc = this->GetVzComponent()->GetCellData();
    }

  vectors = pd->GetVectors();
  vectorsc = cd->GetVectors();
  if ( (vectors == NULL ||
        ((numVectors = vectors->GetNumberOfTuples()) < 1) ) &&
       (vectorsc == NULL ||
        ((numVectorsc = vectorsc->GetNumberOfTuples()) < 1)))
    {
    vtkErrorMacro(<<"No vector data to extract!");
    return 1;
    }

  const char* name;
  if (vectors)
    {
    name = vectors->GetName();
    }
  else if (vectorsc)
    {
    name = vectorsc->GetName();
    }
  else
    {
    name = 0;
    }

  char* newName;
  if (name)
    {
    newName = new char[strlen(name)+10];
    }
  else
    {
    newName = new char[10];
    name = "";
    }

  if (vectors)
    {
    vx = vtkDataArray::CreateDataArray(vectors->GetDataType());
    vx->SetNumberOfTuples(numVectors);
    sprintf(newName, "%s-x", name);
    vx->SetName(newName);
    vy = vtkDataArray::CreateDataArray(vectors->GetDataType());
    vy->SetNumberOfTuples(numVectors);
    sprintf(newName, "%s-y", name);
    vy->SetName(newName);
    vz = vtkDataArray::CreateDataArray(vectors->GetDataType());
    vz->SetNumberOfTuples(numVectors);
    sprintf(newName, "%s-z", name);
    vz->SetName(newName);

    switch (vectors->GetDataType())
      {
      vtkTemplateMacro(
        vtkExtractComponents(numVectors,
                             static_cast<VTK_TT *>(vectors->GetVoidPointer(0)),
                             static_cast<VTK_TT *>(vx->GetVoidPointer(0)),
                             static_cast<VTK_TT *>(vy->GetVoidPointer(0)),
                             static_cast<VTK_TT *>(vz->GetVoidPointer(0))));
      }

    outVx->PassData(pd);
    outVx->AddArray(vx);
    outVx->SetActiveScalars(vx->GetName());
    vx->Delete();

    if (this->ExtractToFieldData)
      {
      outVx->AddArray(vy);
      outVx->AddArray(vz);
      }
    else
      {
      outVy->PassData(pd);
      outVy->AddArray(vy);
      outVy->SetActiveScalars(vy->GetName());

      outVz->PassData(pd);
      outVz->AddArray(vz);
      outVz->SetActiveScalars(vz->GetName());
      }
    vy->Delete();
    vz->Delete();
    }

  if (vectorsc)
    {
    vxc = vtkDataArray::CreateDataArray(vectorsc->GetDataType());
    vxc->SetNumberOfTuples(numVectorsc);
    sprintf(newName, "%s-x", name);
    vxc->SetName(newName);
    vyc = vtkDataArray::CreateDataArray(vectorsc->GetDataType());
    vyc->SetNumberOfTuples(numVectorsc);
    sprintf(newName, "%s-y", name);
    vyc->SetName(newName);
    vzc = vtkDataArray::CreateDataArray(vectorsc->GetDataType());
    vzc->SetNumberOfTuples(numVectorsc);
    sprintf(newName, "%s-z", name);
    vzc->SetName(newName);

    switch (vectorsc->GetDataType())
      {
      vtkTemplateMacro(
        vtkExtractComponents(numVectorsc,
                             static_cast<VTK_TT *>(
                               vectorsc->GetVoidPointer(0)),
                             static_cast<VTK_TT *>(vxc->GetVoidPointer(0)),
                             static_cast<VTK_TT *>(vyc->GetVoidPointer(0)),
                             static_cast<VTK_TT *>(vzc->GetVoidPointer(0))));
      }

    outVxc->PassData(cd);
    outVxc->AddArray(vxc);
    outVxc->SetActiveScalars(vxc->GetName());
    vxc->Delete();

    if (this->ExtractToFieldData)
      {
      outVxc->AddArray(vyc);
      outVxc->AddArray(vzc);
      }
    else
      {
      outVyc->PassData(cd);
      outVyc->AddArray(vyc);
      outVyc->SetActiveScalars(vyc->GetName());

      outVzc->PassData(cd);
      outVzc->AddArray(vzc);
      outVzc->SetActiveScalars(vzc->GetName());
      }
    vyc->Delete();
    vzc->Delete();
    }
  delete[] newName;

  return 1;
}

void vtkExtractVectorComponents::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ExtractToFieldData: " << this->ExtractToFieldData << endl;
}
