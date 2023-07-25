// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkTreeLevelsFilter.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

#include "vtkGraph.h"
#include "vtkTree.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkTreeLevelsFilter);

vtkTreeLevelsFilter::vtkTreeLevelsFilter() = default;

int vtkTreeLevelsFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Storing the inputTree and outputTree handles
  vtkTree* inputTree = vtkTree::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkTree* outputTree = vtkTree::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Copy the input to the output.
  outputTree->ShallowCopy(inputTree);

  // Add the 1-tuple array that will store the level from
  // the root down (root = 0, and +1 for each level down)
  vtkIntArray* levelArray = vtkIntArray::New();
  levelArray->SetName("level");
  levelArray->SetNumberOfComponents(1);
  levelArray->SetNumberOfTuples(outputTree->GetNumberOfVertices());
  vtkDataSetAttributes* data = outputTree->GetVertexData();
  data->AddArray(levelArray);

  // Add the 1-tuple array that will marks each
  // leaf with a '1' and everything else with a '0'
  vtkIntArray* leafArray = vtkIntArray::New();
  leafArray->SetName("leaf");
  leafArray->SetNumberOfComponents(1);
  leafArray->SetNumberOfTuples(outputTree->GetNumberOfVertices());
  data->AddArray(leafArray);

  for (vtkIdType i = 0; i < outputTree->GetNumberOfVertices(); i++)
  {
    levelArray->SetValue(i, outputTree->GetLevel(i));
    leafArray->SetValue(i, outputTree->IsLeaf(i));
  }

  // Set levels as the active point scalar
  data->SetActiveScalars("level");

  // Clean up
  levelArray->Delete();
  leafArray->Delete();

  return 1;
}

void vtkTreeLevelsFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
