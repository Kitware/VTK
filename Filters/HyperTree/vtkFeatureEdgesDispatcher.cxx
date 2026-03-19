// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkFeatureEdgesDispatcher.h"

#include "vtkExecutive.h"
#include "vtkFeatureEdges.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridFeatureEdges.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkFeatureEdgesDispatcher);

//-----------------------------------------------------------------------------
void vtkFeatureEdgesDispatcher::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "Feature Angle: " << this->FeatureAngle << std::endl;
  os << indent << "Boundary Edges: " << (this->BoundaryEdges ? "On" : "Off") << std::endl;
  os << indent << "Feature Edges: " << (this->FeatureEdges ? "On" : "Off") << std::endl;
  os << indent << "Non-Manifold Edges: " << (this->NonManifoldEdges ? "On" : "Off") << std::endl;
  os << indent << "Manifold Edges: " << (this->ManifoldEdges ? "On" : "Off") << std::endl;
  os << indent << "Coloring: " << (this->Coloring ? "On" : "Off") << std::endl;
  os << indent << "Merge Points: " << (this->MergePoints ? "On" : "Off") << std::endl;
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
int vtkFeatureEdgesDispatcher::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkPolyData* output = vtkPolyData::GetData(outInfo);

  if (output)
  {
    if (auto inputPD = vtkPolyData::GetData(inInfo))
    {
      vtkNew<vtkFeatureEdges> featureEdges;
      featureEdges->SetInputData(inputPD);
      featureEdges->SetBoundaryEdges(this->GetBoundaryEdges());
      featureEdges->SetFeatureEdges(this->GetFeatureEdges());
      featureEdges->SetFeatureAngle(this->GetFeatureAngle());
      featureEdges->SetNonManifoldEdges(this->GetNonManifoldEdges());
      featureEdges->SetManifoldEdges(this->GetManifoldEdges());
      featureEdges->SetColoring(this->GetColoring());

      if (featureEdges->GetExecutive()->Update())
      {
        output->ShallowCopy(featureEdges->GetOutput(0));
        return 1;
      }
    }
    else if (auto inputHTG = vtkHyperTreeGrid::GetData(inInfo))
    {
      vtkNew<vtkHyperTreeGridFeatureEdges> htgFeatureEdges;
      htgFeatureEdges->SetInputData(inputHTG);
      htgFeatureEdges->SetMergePoints(this->GetMergePoints());

      if (htgFeatureEdges->GetExecutive()->Update())
      {
        output->ShallowCopy(htgFeatureEdges->GetOutput(0));
        return 1;
      }
    }
  }
  vtkErrorMacro(<< "Unable to retrieve input / output as supported type.");
  return 0;
}

//-----------------------------------------------------------------------------
int vtkFeatureEdgesDispatcher::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
  return 1;
}

VTK_ABI_NAMESPACE_END
