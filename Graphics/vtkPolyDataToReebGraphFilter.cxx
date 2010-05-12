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
#include  "vtkPolyDataToReebGraphFilter.h"

#include  "vtkElevationFilter.h"
#include  "vtkInformation.h"
#include  "vtkInformationVector.h"

vtkCxxRevisionMacro(vtkPolyDataToReebGraphFilter, "$Revision$");
vtkStandardNewMacro(vtkPolyDataToReebGraphFilter);

//----------------------------------------------------------------------------
vtkPolyDataToReebGraphFilter::vtkPolyDataToReebGraphFilter()
{
  FieldId = 0;
  this->SetNumberOfInputPorts(1);
}

//----------------------------------------------------------------------------
vtkPolyDataToReebGraphFilter::~vtkPolyDataToReebGraphFilter()
{
}

//----------------------------------------------------------------------------
int vtkPolyDataToReebGraphFilter::FillInputPortInformation(
  int portNumber, vtkInformation *info)
{
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  return 1;
}

//----------------------------------------------------------------------------
int vtkPolyDataToReebGraphFilter::FillOutputPortInformation(
  int, vtkInformation *info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkReebGraph");
  return 1;
}

//----------------------------------------------------------------------------
void vtkPolyDataToReebGraphFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
vtkReebGraph* vtkPolyDataToReebGraphFilter::GetOutput()
{
  return vtkReebGraph::SafeDownCast(this->GetOutputDataObject(0));
}

//----------------------------------------------------------------------------
int vtkPolyDataToReebGraphFilter::RequestDataObject(vtkInformation *request,
                                            vtkInformationVector **inputVector,
                                            vtkInformationVector *outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
    {
    return 0;
    }
  vtkPolyData *input = vtkPolyData::SafeDownCast(
    inInfo->Get(vtkPolyData::DATA_OBJECT()));

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
int vtkPolyDataToReebGraphFilter::RequestData(vtkInformation*,
                                    vtkInformationVector** inputVector,
                                    vtkInformationVector* outputVector)
{

  vtkInformation *mInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPolyData *mesh = vtkPolyData::SafeDownCast(
                                  mInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkReebGraph *rg = vtkReebGraph::SafeDownCast(
                                  outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if(rg->Build(mesh, FieldId)){
    vtkElevationFilter *f = vtkElevationFilter::New();
    f->SetInput(mesh);
    f->Update();
    vtkPolyData *elevatedMesh = (vtkPolyData *) f->GetOutput();
    rg->Build(elevatedMesh, "Elevation");
  }

  return 1;
}
