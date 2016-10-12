/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataToReebGraphFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPolyDataToReebGraphFilter.h"

#include "vtkElevationFilter.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPointData.h"
#include "vtkReebGraph.h"

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
int vtkPolyDataToReebGraphFilter::FillInputPortInformation(int vtkNotUsed(portNumber), vtkInformation *info)
{
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  return 1;
}

//----------------------------------------------------------------------------
int vtkPolyDataToReebGraphFilter::FillOutputPortInformation(
  int, vtkInformation *info)
{
  info->Set(vtkDirectedGraph::DATA_TYPE_NAME(), "vtkReebGraph");
  return 1;
}

//----------------------------------------------------------------------------
void vtkPolyDataToReebGraphFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Field Id: " << this->FieldId << "\n";
}

//----------------------------------------------------------------------------
vtkReebGraph* vtkPolyDataToReebGraphFilter::GetOutput()
{
  return vtkReebGraph::SafeDownCast(this->GetOutputDataObject(0));
}

//----------------------------------------------------------------------------
int vtkPolyDataToReebGraphFilter::RequestData(vtkInformation*,
                                    vtkInformationVector** inputVector,
                                    vtkInformationVector* outputVector)
{

  vtkInformation  *inInfo = inputVector[0]->GetInformationObject(0);

  vtkPolyData *input = vtkPolyData::SafeDownCast(
    inInfo->Get(vtkPolyData::DATA_OBJECT()));

  vtkInformation  *outInfo = outputVector->GetInformationObject(0);
  vtkReebGraph    *output = vtkReebGraph::SafeDownCast(
    outInfo->Get(vtkReebGraph::DATA_OBJECT()));

  // check for the presence of a scalar field
  vtkDataArray *scalarField = input->GetPointData()->GetArray(FieldId);
  if(!scalarField)
  {
    vtkElevationFilter* eFilter = vtkElevationFilter::New();
    eFilter->SetInputData(input);
    eFilter->Update();
    output->Build(vtkPolyData::SafeDownCast(eFilter->GetOutput()),
                  "Elevation");
    eFilter->Delete();
  }
  else
  {
    output->Build(input, FieldId);
  }
  if(scalarField)
  {
  }
  else
  {
  }

  return 1;
}
