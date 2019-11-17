/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLocationSelector.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLocationSelector.h"

#include "vtkDataSetAttributes.h"
#include "vtkGenericCell.h"
#include "vtkInformation.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkSMPTools.h"
#include "vtkSelectionNode.h"
#include "vtkSignedCharArray.h"
#include "vtkStaticPointLocator.h"
#include "vtkUnstructuredGrid.h"

#include <cassert>

class vtkLocationSelector::vtkInternals
{
public:
  vtkInternals(vtkDataArray* selList)
    : SelectionList(selList)
  {
  }

  virtual ~vtkInternals() {}
  virtual bool Execute(vtkDataSet* dataset, vtkSignedCharArray* insidednessArray) = 0;

protected:
  vtkSmartPointer<vtkDataArray> SelectionList;
};

class vtkLocationSelector::vtkInternalsForPoints : public vtkLocationSelector::vtkInternals
{
public:
  vtkInternalsForPoints(vtkDataArray* selList, double searchRadius)
    : vtkInternals(selList)
    , SearchRadius(searchRadius)
  {
  }

  bool Execute(vtkDataSet* dataset, vtkSignedCharArray* insidednessArray) override
  {
    const vtkIdType numPoints = dataset->GetNumberOfPoints();
    if (numPoints <= 0)
    {
      return false;
    }

    vtkSmartPointer<vtkStaticPointLocator> locator;

    if (dataset->IsA("vtkPointSet"))
    {
      locator = vtkSmartPointer<vtkStaticPointLocator>::New();
      locator->SetDataSet(dataset);
      locator->Update();
    }

    std::fill_n(insidednessArray->GetPointer(0), numPoints, static_cast<char>(0));
    const double radius = this->SearchRadius;

    // Find points closest to each point in the locations of interest.
    vtkIdType numLocations = this->SelectionList->GetNumberOfTuples();
    for (vtkIdType locationId = 0; locationId < numLocations; ++locationId)
    {
      double location[3], dist2;
      this->SelectionList->GetTuple(locationId, location);

      vtkIdType ptId = -1;
      if (locator)
      {
        ptId = locator->FindClosestPointWithinRadius(radius, location, dist2);
      }
      else
      {
        ptId = dataset->FindPoint(location);
        if (ptId >= 0)
        {
          double* x = dataset->GetPoint(ptId);
          double distance = vtkMath::Distance2BetweenPoints(x, location);
          if (distance > radius * radius)
          {
            ptId = -1;
          }
        }
      }

      if (ptId >= 0)
      {
        insidednessArray->SetTypedComponent(ptId, 0, 1);
      }
    }

    insidednessArray->Modified();
    return true;
  }

protected:
  double SearchRadius;
};

class vtkLocationSelector::vtkInternalsForCells : public vtkLocationSelector::vtkInternals
{
public:
  vtkInternalsForCells(vtkDataArray* selList)
    : vtkInternals(selList)
  {
  }

  bool Execute(vtkDataSet* dataset, vtkSignedCharArray* insidednessArray) override
  {
    const auto numLocations = this->SelectionList->GetNumberOfTuples();
    const auto numCells = insidednessArray->GetNumberOfTuples();
    std::fill_n(insidednessArray->GetPointer(0), numCells, static_cast<char>(0));
    std::vector<double> weights(dataset->GetMaxCellSize(), 0.0);
    vtkNew<vtkGenericCell> cell;
    for (vtkIdType cc = 0; cc < numLocations; ++cc)
    {
      double coords[3];
      this->SelectionList->GetTuple(cc, coords);
      int subId;

      double pcoords[3];
      auto cid = dataset->FindCell(coords, nullptr, cell, 0, 0.0, subId, pcoords, &weights[0]);
      if (cid >= 0 && cid < numCells)
      {
        insidednessArray->SetValue(cid, 1);
      }
    }
    insidednessArray->Modified();
    return true;
  }
};

vtkStandardNewMacro(vtkLocationSelector);
//----------------------------------------------------------------------------
vtkLocationSelector::vtkLocationSelector()
  : Internals(nullptr)
{
}

//----------------------------------------------------------------------------
vtkLocationSelector::~vtkLocationSelector() {}

//----------------------------------------------------------------------------
void vtkLocationSelector::Initialize(vtkSelectionNode* node)
{
  this->Superclass::Initialize(node);

  this->Internals.reset();

  auto selectionList = vtkDataArray::SafeDownCast(node->GetSelectionList());
  if (!selectionList || selectionList->GetNumberOfTuples() == 0)
  {
    // empty selection list, nothing to do.
    return;
  }

  if (selectionList->GetNumberOfComponents() != 3)
  {
    vtkErrorMacro("Only 3-d locations are current supported.");
    return;
  }

  if (node->GetContentType() != vtkSelectionNode::LOCATIONS)
  {
    vtkErrorMacro("vtkLocationSelector only supported vtkSelectionNode::LOCATIONS. `"
      << node->GetContentType() << "` is not supported.");
    return;
  }

  const int fieldType = node->GetFieldType();
  const int assoc = vtkSelectionNode::ConvertSelectionFieldToAttributeType(fieldType);

  double radius = node->GetProperties()->Has(vtkSelectionNode::EPSILON())
    ? node->GetProperties()->Get(vtkSelectionNode::EPSILON())
    : 0.0;
  switch (assoc)
  {
    case vtkDataObject::FIELD_ASSOCIATION_POINTS:
      this->Internals.reset(new vtkInternalsForPoints(selectionList, radius));
      break;

    case vtkDataObject::FIELD_ASSOCIATION_CELLS:
      this->Internals.reset(new vtkInternalsForCells(selectionList));
      break;

    default:
      vtkErrorMacro(
        "vtkLocationSelector does not support requested field type `" << fieldType << "`.");
      break;
  }
}

//----------------------------------------------------------------------------
void vtkLocationSelector::Finalize()
{
  this->Internals.reset();
}

//----------------------------------------------------------------------------
bool vtkLocationSelector::ComputeSelectedElements(
  vtkDataObject* input, vtkSignedCharArray* insidednessArray)
{
  assert(input != nullptr && insidednessArray != nullptr);
  vtkDataSet* ds = vtkDataSet::SafeDownCast(input);
  return (this->Internals != nullptr && ds != nullptr)
    ? this->Internals->Execute(ds, insidednessArray)
    : false;
}

//----------------------------------------------------------------------------
void vtkLocationSelector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
