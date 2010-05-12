/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include  "vtkReebGraphToReebGraphPersistenceFilter.h"

#include  "vtkInformation.h"
#include  "vtkInformationVector.h"

vtkCxxRevisionMacro(vtkReebGraphToReebGraphPersistenceFilter, "$Revision$");
vtkStandardNewMacro(vtkReebGraphToReebGraphPersistenceFilter);

//----------------------------------------------------------------------------
vtkReebGraphToReebGraphPersistenceFilter::vtkReebGraphToReebGraphPersistenceFilter()
{
  this->SetNumberOfInputPorts(1);
  this->PersistenceThreshold = 0;
}

//----------------------------------------------------------------------------
vtkReebGraphToReebGraphPersistenceFilter::~vtkReebGraphToReebGraphPersistenceFilter()
{
}

//----------------------------------------------------------------------------
int vtkReebGraphToReebGraphPersistenceFilter::FillInputPortInformation(
  int portNumber, vtkInformation *info)
{
  if(!portNumber){
    info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkReebGraph");
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkReebGraphToReebGraphPersistenceFilter::FillOutputPortInformation(
  int, vtkInformation *info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkReebGraph");
  return 1;
}

//----------------------------------------------------------------------------
void vtkReebGraphToReebGraphPersistenceFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
vtkReebGraph* vtkReebGraphToReebGraphPersistenceFilter::GetOutput()
{
  return vtkReebGraph::SafeDownCast(this->GetOutputDataObject(0));
}

//----------------------------------------------------------------------------
int vtkReebGraphToReebGraphPersistenceFilter::RequestData(
  vtkInformation *request, vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  if (!inInfo)
    {
    return 0;
    }
  vtkReebGraph *input = vtkReebGraph::SafeDownCast(
    inInfo->Get(vtkReebGraph::DATA_OBJECT()));

  if (input)
    {
    vtkInformation *outInfo = outputVector->GetInformationObject(0);
    vtkReebGraph *output = vtkReebGraph::SafeDownCast(
      outInfo->Get(vtkReebGraph::DATA_OBJECT()));

    if(output){
      output->DeepCopy(input);
      output->FilterByPersistence(this->PersistenceThreshold);
    }

    if(!output){
      output = vtkReebGraph::New();
      output->DeepCopy(input);
      output->FilterByPersistence(this->PersistenceThreshold);
      output->SetPipelineInformation(outInfo);
      output->Delete();
    }

    return 1;
    }
  return 0;
}
