// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHyperTreeGridGeometry.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataSetAttributes.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridGeometry1DImpl.h"
#include "vtkHyperTreeGridGeometry2DImpl.h"
#include "vtkHyperTreeGridGeometry3DImpl.h"
#include "vtkHyperTreeGridGeometryImpl.h"
#include "vtkInformation.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"

#include <limits>
#include <memory>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkHyperTreeGridGeometry);

//------------------------------------------------------------------------------
void vtkHyperTreeGridGeometry::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "PassThroughCellIds: " << this->PassThroughCellIds << endl;
  os << indent << "OriginalCellIdArrayName: " << this->OriginalCellIdArrayName << endl;
  os << indent << "Merging: " << this->Merging << endl;
}

//------------------------------------------------------------------------------
int vtkHyperTreeGridGeometry::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
  return 1;
}

//------------------------------------------------------------------------------
int vtkHyperTreeGridGeometry::ProcessTrees(vtkHyperTreeGrid* input, vtkDataObject* outputDO)
{
  // Downcast output data object to polygonal data set
  vtkPolyData* output = vtkPolyData::SafeDownCast(outputDO);
  if (!output)
  {
    vtkErrorMacro("Incorrect type of output: " << outputDO->GetClassName());
    return 0;
  }

  // Retrieve useful grid parameters for speed of access
  unsigned int dimension = input->GetDimension();

  // Initialize output cell data
  this->InData = input->GetCellData();
  this->OutData = output->GetCellData();
  this->OutData->CopyAllOn(); // Should be set before CopyAllocate to be taken into account
  this->OutData->CopyAllocate(this->InData);

  vtkNew<vtkPoints> outPoints;
  vtkNew<vtkCellArray> outCells;

  std::unique_ptr<vtkHyperTreeGridGeometryImpl> implementation;

  // Create a custom internal class depending on the dimension of the input HTG.
  switch (dimension)
  {
    case 1:
      implementation = std::unique_ptr<vtkHyperTreeGridGeometry1DImpl>(
        new vtkHyperTreeGridGeometry1DImpl(input, outPoints, outCells, this->InData, this->OutData,
          this->PassThroughCellIds, this->OriginalCellIdArrayName, this->FillMaterial));
      break;
    case 2:
      implementation = std::unique_ptr<vtkHyperTreeGridGeometry2DImpl>(
        new vtkHyperTreeGridGeometry2DImpl(input, outPoints, outCells, this->InData, this->OutData,
          this->PassThroughCellIds, this->OriginalCellIdArrayName, this->FillMaterial));
      break;
    case 3:
      implementation =
        std::unique_ptr<vtkHyperTreeGridGeometry3DImpl>(new vtkHyperTreeGridGeometry3DImpl(
          this->Merging, input, outPoints, outCells, this->InData, this->OutData,
          this->PassThroughCellIds, this->OriginalCellIdArrayName, this->FillMaterial));
      break;
    default:
      vtkErrorMacro("Incorrect dimension of input: " << dimension);
      return 0;
  } // switch ( dimension )

  // Execute
  implementation->GenerateGeometry();

  // Set output geometry and topology
  output->SetPoints(outPoints);
  if (dimension == 1 || !this->FillMaterial)
  {
    output->SetLines(outCells);
  }
  else
  {
    output->SetPolys(outCells);
  }

  return 1;
}

VTK_ABI_NAMESPACE_END
