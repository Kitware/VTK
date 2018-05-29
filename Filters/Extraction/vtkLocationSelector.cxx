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
#include "vtkInformation.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkSMPTools.h"
#include "vtkSelectionNode.h"
#include "vtkSignedCharArray.h"
#include "vtkStaticCellLocator.h"
#include "vtkStaticPointLocator.h"
#include "vtkUnstructuredGrid.h"

#include <cassert>

class vtkLocationSelector::vtkInternals
{
public:
  virtual ~vtkInternals() {}
  virtual bool Execute(vtkDataSet* dataset, vtkSignedCharArray* insidednessArray) = 0;
};

/// When selecting points near the chosen locations, we use the following
/// strategy:
/// 1. Build a vtkStaticPointLocator using the location of interest.
/// 2. Iterate over all point in the target dataset (dataset being selected) and
///    which if it's within search-radius for the selection locations.
class vtkLocationSelector::vtkInternalsForPoints : public vtkLocationSelector::vtkInternals
{
  vtkNew<vtkStaticPointLocator> PointLocator;
  double SearchRadius;

public:
  vtkInternalsForPoints(vtkDataArray* selList, double searchRadius)
    : SearchRadius(searchRadius)
  {
    vtkNew<vtkUnstructuredGrid> ds;
    vtkNew<vtkPoints> pts;
    pts->SetData(selList);
    ds->SetPoints(pts);
    this->PointLocator->SetDataSet(ds);
    this->PointLocator->Update();
  }

  bool Execute(vtkDataSet* dataset, vtkSignedCharArray* insidednessArray) override
  {
    const vtkIdType numpts = dataset->GetNumberOfPoints();
    if (numpts <= 0)
    {
      return false;
    }

    const double radius = this->SearchRadius;
    vtkStaticPointLocator* locator = this->PointLocator;
    vtkSMPTools::For(0, numpts, [=](vtkIdType begin, vtkIdType end) {
      for (vtkIdType cc = begin; cc < end; ++cc)
      {
        double coords[3], d2;
        dataset->GetPoint(cc, coords);
        auto pid = locator->FindClosestPointWithinRadius(radius, coords, d2);
        insidednessArray->SetValue(cc, pid >= 0 ? 1 : 0);
      }
    });
    insidednessArray->Modified();
    return true;
  }
};

class vtkLocationSelector::vtkInternalsForCells : public vtkLocationSelector::vtkInternals
{
  vtkSmartPointer<vtkDataArray> SelectionList;

public:
  vtkInternalsForCells(vtkDataArray* selList, double vtkNotUsed(searchRadius))
    : SelectionList(selList)
  {
  }

  bool Execute(vtkDataSet* dataset, vtkSignedCharArray* insidednessArray) override
  {
    vtkNew<vtkStaticCellLocator> cellLocator;
    cellLocator->SetDataSet(dataset);
    cellLocator->Update();

    const auto numLocations = this->SelectionList->GetNumberOfTuples();
    const auto numCells = insidednessArray->GetNumberOfTuples();
    std::fill_n(insidednessArray->GetPointer(0), numCells, static_cast<char>(0));
    for (vtkIdType cc = 0; cc < numLocations; ++cc)
    {
      double coords[3];
      this->SelectionList->GetTuple(cc, coords);
      auto cid = cellLocator->FindCell(coords);
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
vtkLocationSelector::~vtkLocationSelector()
{
}

//----------------------------------------------------------------------------
void vtkLocationSelector::Initialize(vtkSelectionNode* node, const std::string& insidednessArrayName)
{
  this->Superclass::Initialize(node, insidednessArrayName);

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
      this->Internals.reset(new vtkInternalsForCells(selectionList, radius));
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
bool vtkLocationSelector::ComputeSelectedElementsForBlock(vtkDataObject* input,
  vtkSignedCharArray* insidednessArray, unsigned int vtkNotUsed(compositeIndex),
  unsigned int vtkNotUsed(amrLevel), unsigned int vtkNotUsed(amrIndex))
{
  assert(input != nullptr && insidednessArray != nullptr);
  vtkDataSet* ds = vtkDataSet::SafeDownCast(input);
  return (this->Internals != nullptr && ds != nullptr) ? this->Internals->Execute(ds, insidednessArray)
                                                       : false;
}

//----------------------------------------------------------------------------
void vtkLocationSelector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
