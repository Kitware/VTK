// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkUniformGrid.h"

#include "vtkAMRBox.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkUnsignedCharArray.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkUniformGrid);

//------------------------------------------------------------------------------
vtkUniformGrid::vtkUniformGrid() = default;

//------------------------------------------------------------------------------
vtkUniformGrid::~vtkUniformGrid() = default;

//------------------------------------------------------------------------------
void vtkUniformGrid::Initialize()
{
  this->Superclass::Initialize();
}

//------------------------------------------------------------------------------
int vtkUniformGrid::Initialize(const vtkAMRBox* def, double* origin, double* spacing)
{
  if (def->Empty())
  {
    vtkWarningMacro("Can't construct a data set from an empty box.");
    return 0;
  }
  if (def->ComputeDimension() == 2)
  {
    // NOTE: Define it 3D, with the third dim 0. eg. (X,X,0)(X,X,0)
    vtkWarningMacro("Can't construct a 3D data set from a 2D box.");
    return 0;
  }

  this->Initialize();
  int nPoints[3];
  def->GetNumberOfNodes(nPoints);

  this->SetDimensions(nPoints);
  this->SetSpacing(spacing);
  this->SetOrigin(origin);

  return 1;
}

//------------------------------------------------------------------------------
int vtkUniformGrid::Initialize(
  const vtkAMRBox* def, double* origin, double* spacing, int nGhostsI, int nGhostsJ, int nGhostsK)
{
  if (!this->Initialize(def, origin, spacing))
  {
    return 0;
  }

  // Generate ghost cell array, with no ghosts marked.
  int nCells[3];
  def->GetNumberOfCells(nCells);
  vtkNew<vtkUnsignedCharArray> ghosts;
  this->GetCellData()->AddArray(ghosts);
  ghosts->SetName(vtkDataSetAttributes::GhostArrayName());
  ghosts->SetNumberOfComponents(1);
  ghosts->SetNumberOfTuples(nCells[0] * nCells[1] * nCells[2]);
  ghosts->FillValue(0);
  // If there are ghost cells mark them.
  if (nGhostsI || nGhostsJ || nGhostsK)
  {
    unsigned char* pG = ghosts->GetPointer(0);
    const int* lo = def->GetLoCorner();
    const int* hi = def->GetHiCorner();
    // Identify & fill ghost regions
    if (nGhostsI)
    {
      const vtkAMRBox left(lo[0], lo[1], lo[2], lo[0] + nGhostsI - 1, hi[1], hi[2]);
      FillRegion(pG, *def, left, static_cast<unsigned char>(1));
      const vtkAMRBox right(hi[0] - nGhostsI + 1, lo[1], lo[2], hi[0], hi[1], hi[2]);
      FillRegion(pG, *def, right, static_cast<unsigned char>(1));
    }
    if (nGhostsJ)
    {
      const vtkAMRBox front(lo[0], lo[1], lo[2], hi[0], lo[1] + nGhostsJ - 1, hi[2]);
      FillRegion(pG, *def, front, static_cast<unsigned char>(1));
      const vtkAMRBox back(lo[0], hi[1] - nGhostsJ + 1, lo[2], hi[0], hi[1], hi[2]);
      FillRegion(pG, *def, back, static_cast<unsigned char>(1));
    }
    if (nGhostsK)
    {
      const vtkAMRBox bottom(lo[0], lo[1], lo[2], hi[0], hi[1], lo[2] + nGhostsK - 1);
      FillRegion(pG, *def, bottom, static_cast<unsigned char>(1));
      const vtkAMRBox top(lo[0], lo[1], hi[2] - nGhostsK + 1, hi[0], hi[1], hi[2]);
      FillRegion(pG, *def, top, static_cast<unsigned char>(1));
    }
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkUniformGrid::Initialize(
  const vtkAMRBox* def, double* origin, double* spacing, const int nGhosts[3])
{
  return this->Initialize(def, origin, spacing, nGhosts[0], nGhosts[1], nGhosts[2]);
}

//------------------------------------------------------------------------------
int vtkUniformGrid::Initialize(const vtkAMRBox* def, double* origin, double* spacing, int nGhosts)
{
  return this->Initialize(def, origin, spacing, nGhosts, nGhosts, nGhosts);
}

//------------------------------------------------------------------------------
// Copy the geometric and topological structure of an input structured points
// object.
void vtkUniformGrid::CopyStructure(vtkDataSet* ds)
{
  this->Initialize();

  this->Superclass::CopyStructure(ds);
}

//------------------------------------------------------------------------------
void vtkUniformGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkImageData* vtkUniformGrid::NewImageDataCopy()
{
  vtkImageData* copy = vtkImageData::New();

  copy->ShallowCopy(this);

  double origin[3];
  double spacing[3];
  this->GetOrigin(origin);
  this->GetSpacing(spacing);
  // First set the extent of the copy to empty so that
  // the next call computes the DataDescription for us
  copy->SetExtent(0, -1, 0, -1, 0, -1);
  copy->SetExtent(this->GetExtent());
  copy->SetOrigin(origin);
  copy->SetSpacing(spacing);

  return copy;
}

//------------------------------------------------------------------------------
// Override this method because of blanking
void vtkUniformGrid::ComputeScalarRange()
{
  if (this->GetMTime() > this->ScalarRangeComputeTime)
  {
    vtkDataArray* ptScalars = this->PointData->GetScalars();
    vtkDataArray* cellScalars = this->CellData->GetScalars();
    double ptRange[2];
    double cellRange[2];
    double s;

    ptRange[0] = VTK_DOUBLE_MAX;
    ptRange[1] = VTK_DOUBLE_MIN;
    if (ptScalars)
    {
      vtkIdType num = this->GetNumberOfPoints();
      for (vtkIdType id = 0; id < num; ++id)
      {
        if (this->IsPointVisible(id))
        {
          s = ptScalars->GetComponent(id, 0);
          if (s < ptRange[0])
          {
            ptRange[0] = s;
          }
          if (s > ptRange[1])
          {
            ptRange[1] = s;
          }
        }
      }
    }

    cellRange[0] = ptRange[0];
    cellRange[1] = ptRange[1];
    if (cellScalars)
    {
      vtkIdType num = this->GetNumberOfCells();
      for (vtkIdType id = 0; id < num; ++id)
      {
        if (this->IsCellVisible(id))
        {
          s = cellScalars->GetComponent(id, 0);
          if (s < cellRange[0])
          {
            cellRange[0] = s;
          }
          if (s > cellRange[1])
          {
            cellRange[1] = s;
          }
        }
      }
    }

    this->ScalarRange[0] = (cellRange[0] >= VTK_DOUBLE_MAX ? 0.0 : cellRange[0]);
    this->ScalarRange[1] = (cellRange[1] <= VTK_DOUBLE_MIN ? 1.0 : cellRange[1]);
    this->ScalarRangeComputeTime.Modified();
  }
}

//------------------------------------------------------------------------------
vtkUniformGrid* vtkUniformGrid::GetData(vtkInformation* info)
{
  return info ? vtkUniformGrid::SafeDownCast(info->Get(DATA_OBJECT())) : nullptr;
}

//------------------------------------------------------------------------------
vtkUniformGrid* vtkUniformGrid::GetData(vtkInformationVector* v, int i)
{
  return vtkUniformGrid::GetData(v->GetInformationObject(i));
}
VTK_ABI_NAMESPACE_END
