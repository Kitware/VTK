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
#include  "vtkUnstructuredGridToReebGraphFilter.h"

#include  "vtkElevationFilter.h"
#include  "vtkInformation.h"
#include  "vtkInformationVector.h"

vtkCxxRevisionMacro(vtkUnstructuredGridToReebGraphFilter, "$Revision$");
vtkStandardNewMacro(vtkUnstructuredGridToReebGraphFilter);

//----------------------------------------------------------------------------
vtkUnstructuredGridToReebGraphFilter::vtkUnstructuredGridToReebGraphFilter()
{
  FieldId = 0;
  this->SetNumberOfInputPorts(1);
}

//----------------------------------------------------------------------------
vtkUnstructuredGridToReebGraphFilter::~vtkUnstructuredGridToReebGraphFilter()
{
}

//----------------------------------------------------------------------------
int vtkUnstructuredGridToReebGraphFilter::FillInputPortInformation(
  int portNumber, vtkInformation *info)
{
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
  return 1;
}

//----------------------------------------------------------------------------
int vtkUnstructuredGridToReebGraphFilter::FillOutputPortInformation(
  int, vtkInformation *info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkReebGraph");
  return 1;
}

//----------------------------------------------------------------------------
void vtkUnstructuredGridToReebGraphFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
vtkReebGraph* vtkUnstructuredGridToReebGraphFilter::GetOutput()
{
  return vtkReebGraph::SafeDownCast(this->GetOutputDataObject(0));
}

//----------------------------------------------------------------------------
int vtkUnstructuredGridToReebGraphFilter::RequestDataObject(vtkInformation *request,
                                            vtkInformationVector **inputVector,
                                            vtkInformationVector *outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
    {
    return 0;
    }
  vtkUnstructuredGrid *input = vtkUnstructuredGrid::SafeDownCast(
    inInfo->Get(vtkUnstructuredGrid::DATA_OBJECT()));

  if (input)
    {
    vtkInformation *outInfo = outputVector->GetInformationObject(0);
    vtkReebGraph *output = (vtkReebGraph *)
      outInfo->Get(vtkReebGraph::DATA_OBJECT());

    if(!output){
      output = vtkReebGraph::New();
      output->SetPipelineInformation(outInfo);
    }
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkUnstructuredGridToReebGraphFilter::RequestData(vtkInformation*,
                                    vtkInformationVector** inputVector,
                                    vtkInformationVector* outputVector)
{

  vtkInformation *mInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkUnstructuredGrid *mesh = vtkUnstructuredGrid::SafeDownCast(
                                  mInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkReebGraph *rg = vtkReebGraph::SafeDownCast(
                                  outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if(rg->Build(mesh, FieldId)){
    vtkElevationFilter *f = vtkElevationFilter::New();
    f->SetInput(mesh);
    f->Update();
    vtkUnstructuredGrid *elevatedMesh = (vtkUnstructuredGrid *) f->GetOutput();
    rg->Build(elevatedMesh, "Elevation");
  }

  return 1;
}
