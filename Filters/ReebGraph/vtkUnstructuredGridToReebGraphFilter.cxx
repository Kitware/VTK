// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkUnstructuredGridToReebGraphFilter.h"

#include "vtkElevationFilter.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkReebGraph.h"
#include "vtkUnstructuredGrid.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkUnstructuredGridToReebGraphFilter);

//------------------------------------------------------------------------------
vtkUnstructuredGridToReebGraphFilter::vtkUnstructuredGridToReebGraphFilter()
{
  FieldId = 0;
  this->SetNumberOfInputPorts(1);
}

//------------------------------------------------------------------------------
vtkUnstructuredGridToReebGraphFilter::~vtkUnstructuredGridToReebGraphFilter() = default;

//------------------------------------------------------------------------------
int vtkUnstructuredGridToReebGraphFilter::FillInputPortInformation(
  int vtkNotUsed(portNumber), vtkInformation* info)
{
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
  return 1;
}

//------------------------------------------------------------------------------
int vtkUnstructuredGridToReebGraphFilter::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDirectedGraph::DATA_TYPE_NAME(), "vtkReebGraph");
  return 1;
}

//------------------------------------------------------------------------------
void vtkUnstructuredGridToReebGraphFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Field Id: " << this->FieldId << "\n";
}

//------------------------------------------------------------------------------
vtkReebGraph* vtkUnstructuredGridToReebGraphFilter::GetOutput()
{
  return vtkReebGraph::SafeDownCast(this->GetOutputDataObject(0));
}

//------------------------------------------------------------------------------
int vtkUnstructuredGridToReebGraphFilter::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  vtkUnstructuredGrid* input =
    vtkUnstructuredGrid::SafeDownCast(inInfo->Get(vtkUnstructuredGrid::DATA_OBJECT()));

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkReebGraph* output = vtkReebGraph::SafeDownCast(outInfo->Get(vtkReebGraph::DATA_OBJECT()));

  // check for the presence of a scalar field
  vtkDataArray* scalarField = input->GetPointData()->GetArray(FieldId);
  if (!scalarField)
  {
    vtkElevationFilter* eFilter = vtkElevationFilter::New();
    eFilter->SetInputData(input);
    eFilter->Update();
    output->Build(vtkUnstructuredGrid::SafeDownCast(eFilter->GetOutput()), "Elevation");
    eFilter->Delete();
  }
  else
  {
    output->Build(input, FieldId);
  }

  return 1;
}
VTK_ABI_NAMESPACE_END
