// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDGCopyResponder.h"

#include "vtkBoundingBox.h"
#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkCellGridCopyQuery.h"
#include "vtkDGHex.h"
#include "vtkDataSetAttributes.h"
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"
#include "vtkStringToken.h"
#include "vtkTypeInt64Array.h"

#include <unordered_set>

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

namespace
{

/// Create a "deep copy" (either empty array or full copy) of sourceArray.
///
/// This returns true if a new array was created and false if it already existed in the array map.
bool deepCopy(vtkCellGridCopyQuery* query, vtkDataArray* sourceArray, vtkDataArray*& targetArray)
{
  bool didCreate = false;
  auto& rewrites = query->GetArrayMap();
  auto it = rewrites.find(sourceArray);
  if (it != rewrites.end())
  {
    targetArray = vtkDataArray::SafeDownCast(it->second);
  }
  else
  {
    targetArray = sourceArray
      ? vtkDataArray::SafeDownCast(vtkAbstractArray::CreateArray(sourceArray->GetDataType()))
      : nullptr;
    didCreate = true;
  }
  if (targetArray)
  {
    // Add the connectivity to the array map
    rewrites[sourceArray] = targetArray;

    if (didCreate)
    {
      if (query->GetCopyArrayValues())
      {
        targetArray->DeepCopy(sourceArray);
      }
      else
      {
        // Copy connectivity "metadata" only.
        if (sourceArray->HasInformation())
        {
          targetArray->CopyInformation(sourceArray->GetInformation(), /*deep*/ 1);
        }
        targetArray->SetName(sourceArray->GetName());
        targetArray->SetNumberOfComponents(sourceArray->GetNumberOfComponents());
        targetArray->CopyComponentNames(sourceArray);
      }
    }
  }
  return didCreate;
}

} // anonymous namespace

vtkStandardNewMacro(vtkDGCopyResponder);

bool vtkDGCopyResponder::Query(
  vtkCellGridCopyQuery* query, vtkCellMetadata* cellType, vtkCellGridResponders* caches)
{
  (void)cellType;
  (void)caches;

  vtkStringToken cellTypeName = cellType->GetClassName();
  auto* sourceMetadata = vtkDGCell::SafeDownCast(query->GetSource()->GetCellType(cellTypeName));

  vtkDGCell* targetMetadata = nullptr;
  if (query->GetCopyCellTypes())
  {
    auto targetMetadataBase = vtkCellMetadata::NewInstance(cellTypeName, query->GetTarget());
    targetMetadata = vtkDGCell::SafeDownCast(targetMetadataBase);

    if (!sourceMetadata || !targetMetadata)
    {
      vtkErrorMacro("Cannot copy non-DG cell with DG responder.");
      return false;
    }

    // If we are copying cells, then ensure the connectivity is copied.
    // If we are copying cells but not values, this will create an empty connectivity array.
    this->CopySpecs(query, sourceMetadata, targetMetadata);

    // Now, copy the arrays for any cell-attributes we are copying and then
    // copy the cell-attribute (or at least update its arrays for the current cell-type).
    if (query->GetCopyOnlyShape())
    {
      query->CopyAttributeArrays(query->GetSource()->GetShapeAttribute(), cellTypeName);
    }
    else
    {
      for (const auto& attId : query->GetCellAttributeIds())
      {
        if (auto* cellAtt = query->GetSource()->GetCellAttributeById(attId))
        {
          query->CopyAttributeArrays(cellAtt, cellTypeName);
        }
      }
    }
  }

  // Finally, create the cell-attributes as needed and add the arrays.
  if (query->GetCopyOnlyShape())
  {
    query->CopyOrUpdateAttributeRecord(query->GetSource()->GetShapeAttribute(), cellTypeName);
  }
  else
  {
    for (const auto& attId : query->GetCellAttributeIds())
    {
      auto* srcAtt = query->GetSource()->GetCellAttributeById(attId);
      if (!srcAtt)
      {
        vtkWarningMacro("No attribute " << attId << " in source.");
        continue;
      }
      query->CopyOrUpdateAttributeRecord(srcAtt, cellTypeName);
    }
  }

  return true;
}

void vtkDGCopyResponder::CopySpecs(
  vtkCellGridCopyQuery* query, vtkDGCell* sourceMetadata, vtkDGCell* targetMetadata)
{
  if (!query->GetCopyCells())
  {
    vtkDGCell::Source empty;
    targetMetadata->GetCellSpec() = empty;
    return;
  }

  this->CopySpec(query, sourceMetadata->GetCellSpec(), targetMetadata->GetCellSpec());
  std::size_t nn = sourceMetadata->GetSideSpecs().size();
  targetMetadata->GetSideSpecs().resize(nn);
  for (std::size_t ii = 0; ii < nn; ++ii)
  {
    this->CopySpec(query, sourceMetadata->GetSideSpecs()[ii], targetMetadata->GetSideSpecs()[ii]);
  }
}

void vtkDGCopyResponder::CopySpec(
  vtkCellGridCopyQuery* query, vtkDGCell::Source& sourceSpec, vtkDGCell::Source& targetSpec)
{
  bool didCreateConn = false;
  bool didCreateNodeGhost = false;
  targetSpec.Blanked = sourceSpec.Blanked;
  targetSpec.Offset = query->GetCopyArrayValues() ? sourceSpec.Offset : 0;
  targetSpec.SourceShape = sourceSpec.SourceShape;
  targetSpec.SideType = sourceSpec.SideType;
  targetSpec.SelectionType = sourceSpec.SelectionType;
  if (query->GetCopyArrayValues() && !query->GetDeepCopyArrays())
  {
    // Copy by reference.
    targetSpec.Connectivity = sourceSpec.Connectivity;
    targetSpec.NodalGhostMarks = sourceSpec.NodalGhostMarks;
  }
  else
  {
    // Deep-copy the connectivity and ghost-node-marks (or use the existing map entry's deep copy).
    vtkDataArray* targetConn = nullptr;
    didCreateConn = deepCopy(query, sourceSpec.Connectivity, targetConn);
    targetSpec.Connectivity = targetConn;

    vtkDataArray* targetNodeGhost = nullptr;
    didCreateNodeGhost = deepCopy(query, sourceSpec.NodalGhostMarks, targetNodeGhost);
    targetSpec.NodalGhostMarks = targetNodeGhost;
  }

  // If we have a non-null connectivity, add it to the
  // same group as its counterpart in the source grid.
  if (targetSpec.Connectivity)
  {
    auto groupId = query->GetSource()->GetAttributeTypeForArray(sourceSpec.Connectivity);
    query->GetTarget()->GetAttributes(groupId)->AddArray(targetSpec.Connectivity);
    if (didCreateConn)
    {
      targetSpec.Connectivity->Delete();
    }
  }

  // If we have a non-null ghost markings, add it to the
  // same group as its counterpart in the source grid.
  if (targetSpec.NodalGhostMarks)
  {
    auto groupId = query->GetSource()->GetAttributeTypeForArray(sourceSpec.NodalGhostMarks);
    query->GetTarget()->GetAttributes(groupId)->AddArray(targetSpec.NodalGhostMarks);
    if (didCreateNodeGhost)
    {
      targetSpec.NodalGhostMarks->Delete();
    }
  }
}

VTK_ABI_NAMESPACE_END
