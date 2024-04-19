// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAMRInterpolatedVelocityField.h"

#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkFieldData.h"
#include "vtkObjectFactory.h"
#include "vtkOverlappingAMR.h"
#include "vtkPointData.h"
#include "vtkUniformGrid.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
namespace
{
bool Inside(double q[3], double gbounds[6])
{
  return gbounds[0] <= q[0] && q[0] <= gbounds[1] && gbounds[2] <= q[1] && q[1] <= gbounds[3] &&
    gbounds[4] <= q[2] && q[2] <= gbounds[5];
}

bool FindInLevel(double q[3], vtkOverlappingAMR* amrds, int level, unsigned int& gridId)
{
  for (unsigned int i = 0; i < amrds->GetNumberOfDataSets(level); i++)
  {
    double gbounds[6];
    amrds->GetBounds(level, i, gbounds);
    bool inside = Inside(q, gbounds);
    if (inside)
    {
      gridId = i;
      return true;
    }
  }
  return false;
}

} // namespace

// The class proper begins here
vtkStandardNewMacro(vtkAMRInterpolatedVelocityField);
vtkCxxSetObjectMacro(vtkAMRInterpolatedVelocityField, AmrDataSet, vtkOverlappingAMR);

//------------------------------------------------------------------------------
vtkAMRInterpolatedVelocityField::vtkAMRInterpolatedVelocityField()
{
  this->Weights.resize(8);
  this->AmrDataSet = nullptr;
  this->LastLevel = this->LastId = -1;
}

//------------------------------------------------------------------------------
vtkAMRInterpolatedVelocityField::~vtkAMRInterpolatedVelocityField()
{
  this->SetAmrDataSet(nullptr);
  this->Weights.clear();
}

//------------------------------------------------------------------------------
// We are ignoring the input parameters to the method as we are going to
// specially initialize AMR velocity fields.
int vtkAMRInterpolatedVelocityField::SelfInitialize()
{
  // Initialize directly called on this velocity field.
  this->InitializationState = vtkAbstractInterpolatedVelocityField::SELF_INITIALIZE;

  // Obtain the vectors
  vtkDataSet* ds = this->LastDataSet;
  vtkDataArray* gVectors = nullptr;
  if (ds != nullptr)
  {
    gVectors = ds->GetAttributesAsFieldData(this->VectorsType)->GetArray(this->VectorsSelection);
  }

  auto datasets = vtkCompositeDataSet::GetDataSets(this->AmrDataSet);

  // Add information into the interpolation function cache. Note that no find cell strategy
  // is required. If no vectors are specified, use the local dataset vectors.
  vtkDataArray* vectors;
  for (auto& dataset : datasets)
  {
    vectors = (gVectors
        ? gVectors
        : vectors =
            dataset->GetAttributesAsFieldData(this->VectorsType)->GetArray(this->VectorsSelection));

    this->AddToDataSetsInfo(dataset, nullptr, vectors);
  }

  // Indicate that the subclass has taken over initialization.
  return 1;
}

//------------------------------------------------------------------------------
bool vtkAMRInterpolatedVelocityField::FindGrid(
  double q[3], vtkOverlappingAMR* amrds, unsigned int& level, unsigned int& gridId)
{
  if (!FindInLevel(q, amrds, 0, gridId))
  {
    return false;
  }

  unsigned int maxLevels = amrds->GetNumberOfLevels();
  for (level = 0; level < maxLevels; level++)
  {
    unsigned int n;
    unsigned int* children = amrds->GetChildren(level, gridId, n);
    if (children == nullptr)
    {
      break;
    }
    unsigned int i;
    for (i = 0; i < n; i++)
    {
      double bb[6];
      amrds->GetBounds(level + 1, children[i], bb);
      if (Inside(q, bb))
      {
        gridId = children[i];
        break;
      }
    }
    if (i >= n)
    {
      break;
    }
  }
  return true;
}

//------------------------------------------------------------------------------
int vtkAMRInterpolatedVelocityField::FunctionValues(double* x, double* f)
{
  if (this->LastDataSet && this->FunctionValues(this->LastDataSet, x, f))
  {
    return 1;
  }

  // Either we do not know which data set it is, or existing LastDataSet does not contain x
  // In any case, set LastDataSet to nullptr and try to find a new one
  this->LastDataSet = nullptr;
  this->LastCellId = -1;
  this->LastLevel = -1;
  this->LastId = -1;

  unsigned int level, gridId;
  if (!FindGrid(x, this->AmrDataSet, level, gridId))
  {
    return 0;
  }
  this->LastLevel = level;
  this->LastId = gridId;

  auto ds = this->AmrDataSet->GetDataSet(level, gridId);
  if (!ds)
  {
    return 0;
  }
  if (!this->FunctionValues(ds, x, f))
  {
    return 0;
  }

  this->LastDataSet = ds;
  return 1;
}

//------------------------------------------------------------------------------
bool vtkAMRInterpolatedVelocityField::SetLastDataSet(int level, int id)
{
  this->LastLevel = level;
  this->LastId = id;
  this->LastDataSet = this->AmrDataSet->GetDataSet(level, id);
  return this->LastDataSet != nullptr;
}

//------------------------------------------------------------------------------
void vtkAMRInterpolatedVelocityField::SetLastCellId(vtkIdType, int)
{
  vtkWarningMacro("Calling SetLastCellId has no effect");
}

//------------------------------------------------------------------------------
bool vtkAMRInterpolatedVelocityField::GetLastDataSetLocation(unsigned int& level, unsigned int& id)
{
  if (this->LastLevel < 0)
  {
    return false;
  }

  level = static_cast<unsigned int>(this->LastLevel);
  id = static_cast<unsigned int>(this->LastId);
  return true;
}

//------------------------------------------------------------------------------
// Copy the list of datasets to copy from.
void vtkAMRInterpolatedVelocityField::CopyParameters(vtkAbstractInterpolatedVelocityField* from)
{
  this->Superclass::CopyParameters(from);

  vtkAMRInterpolatedVelocityField* obj = vtkAMRInterpolatedVelocityField::SafeDownCast(from);
  if (!obj)
  {
    return;
  }

  this->SetAmrDataSet(obj->AmrDataSet);
}

//------------------------------------------------------------------------------
void vtkAMRInterpolatedVelocityField::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
