// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHyperTreeGridGeometry.h"
#include "vtkHyperTreeGridGeometryInternal1D.h"
#include "vtkHyperTreeGridGeometryInternal2D.h"
#include "vtkHyperTreeGridGeometryInternal3D.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataSetAttributes.h"
#include "vtkHyperTreeGrid.h"
#include "vtkInformation.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"

#include <limits>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkHyperTreeGridGeometry);

//------------------------------------------------------------------------------
vtkHyperTreeGridGeometry::vtkHyperTreeGridGeometry()
{
  // Create storage for corners of leaf cells
  this->Points = vtkPoints::New();

  // Create storage for untructured leaf cells
  this->Cells = vtkCellArray::New();

  // Default Locator is 0
  this->Merging = false;

  __trace_htg_geometry = (std::getenv("TRACE") != nullptr);
}

//------------------------------------------------------------------------------
vtkHyperTreeGridGeometry::~vtkHyperTreeGridGeometry()
{
  if (this->Points)
  {
    this->Points->Delete();
    this->Points = nullptr;
  }
  if (this->Cells)
  {
    this->Cells->Delete();
    this->Cells = nullptr;
  }
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGeometry::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->Points)
  {
    os << indent << "Points:\n";
    this->Points->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Points: ( none )\n";
  }

  if (this->Cells)
  {
    os << indent << "Cells:\n";
    this->Cells->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Cells: ( none )\n";
  }

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
  this->OutData->CopyAllocate(this->InData);

  // Create storage for corners of leaf cells
  if (this->Points)
  {
    this->Points->Delete();
  }
  this->Points = vtkPoints::New();

  // Create storage for untructured leaf cells
  if (this->Cells)
  {
    this->Cells->Delete();
  }
  this->Cells = vtkCellArray::New();

  // create custom internal class
  assert(this->Internal == nullptr);
  switch (dimension)
  {
    case 1:
      this->Internal = new vtkHyperTreeGridGeometry::vtkInternal1D("vtkInternal1D", this->Merging,
        input, this->Points, this->Cells, this->InData, this->OutData, this->PassThroughCellIds,
        this->OriginalCellIdArrayName);
      break;
    case 2:
      this->Internal = new vtkHyperTreeGridGeometry::vtkInternal2D("vtkInternal2D", this->Merging,
        input, this->Points, this->Cells, this->InData, this->OutData, this->PassThroughCellIds,
        this->OriginalCellIdArrayName);
      break;
    case 3:
      this->Internal = new vtkHyperTreeGridGeometry::vtkInternal3D("vtkInternal3D", this->Merging,
        input, output, this->Points, this->Cells, this->InData, this->OutData,
        this->PassThroughCellIds, this->OriginalCellIdArrayName);
      break;
    default:
      vtkErrorMacro("Incorrect dimension of input: " << dimension);
      return 0;
  } // switch ( dimension )
  // Set output geometry and topology
  output->SetPoints(this->Points);
  if (dimension == 1)
  {
    output->SetLines(this->Cells);
  }
  else
  {
    output->SetPolys(this->Cells);
  }

  if (this->Points)
  {
    this->Points->Delete();
    this->Points = nullptr;
  }
  if (this->Cells)
  {
    this->Cells->Delete();
    this->Cells = nullptr;
  }

  // clean custom internal class
  if (this->Internal != nullptr)
  {
    delete this->Internal;
  }

  return 1;
}

VTK_ABI_NAMESPACE_END
