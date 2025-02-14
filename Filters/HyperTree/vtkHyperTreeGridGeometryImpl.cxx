// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHyperTreeGridGeometryImpl.h"
#include "vtkBitArray.h"
#include "vtkCellArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkHyperTreeGrid.h"
#include "vtkPoints.h"
#include "vtkUnsignedCharArray.h"

#include <limits>
#include <set>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

//----------------------------------------------------------------------------------------------
vtkHyperTreeGridGeometryImpl::vtkHyperTreeGridGeometryImpl(vtkHyperTreeGrid* input,
  vtkPoints* outPoints, vtkCellArray* outCells, vtkDataSetAttributes* inCellDataAttributes,
  vtkDataSetAttributes* outCellDataAttributes, bool passThroughCellIds,
  const std::string& originalCellIdArrayName, bool fillMaterial)
  : Input(input)
  , OutPoints(outPoints)
  , OutCells(outCells)
  , InCellDataAttributes(inCellDataAttributes)
  , OutCellDataAttributes(outCellDataAttributes)
  , FillMaterial(fillMaterial)
  , PassThroughCellIds(passThroughCellIds)
  , OriginalCellIdArrayName(originalCellIdArrayName)
{
  // Retrieve ghost
  this->InGhostArray = this->Input->GetGhostCells();
  // Retrieve mask
  this->InMaskArray = this->Input->HasMask() ? this->Input->GetMask() : nullptr;
  // Retrieve interface data when relevant
  this->HasInterface = this->Input->GetHasInterface();
  if (this->HasInterface)
  {
    this->InIntercepts =
      this->InCellDataAttributes->GetArray(this->Input->GetInterfaceInterceptsName());
    if (!this->InIntercepts)
    {
      vtkWarningWithObjectMacro(
        nullptr, "vtkInternal HasInterface=true but no interface intercepts");
      this->HasInterface = false;
    }
    this->InNormals = this->InCellDataAttributes->GetArray(this->Input->GetInterfaceNormalsName());
    if (!this->InNormals)
    {
      vtkWarningWithObjectMacro(nullptr, "vtkInternal HasInterface=true but no interface normals");
      this->HasInterface = false;
    }
  }
  this->CellIntercepts.resize(3);
  this->CellNormals.resize(3);
  if (passThroughCellIds && !originalCellIdArrayName.empty())
  {
    vtkNew<vtkIdTypeArray> originalCellIds;
    originalCellIds->SetName(originalCellIdArrayName.c_str());
    originalCellIds->SetNumberOfComponents(1);
    this->OutCellDataAttributes->AddArray(originalCellIds);
  }
}

//----------------------------------------------------------------------------------------------
void vtkHyperTreeGridGeometryImpl::CreateNewCellAndCopyData(
  const std::vector<vtkIdType>& outPointIds, vtkIdType cellId)
{
  // Insert new cell
  vtkIdType outputCellIndex =
    this->OutCells->InsertNextCell(outPointIds.size(), outPointIds.data());

  // Copy the data from the cell this face comes from
  this->OutCellDataAttributes->CopyData(this->InCellDataAttributes, cellId, outputCellIndex);
  // Insert value original VTK cell local index on server
  if (this->PassThroughCellIds && !this->OriginalCellIdArrayName.empty())
  {
    auto typedCellIds = vtkIdTypeArray::SafeDownCast(
      this->OutCellDataAttributes->GetArray(this->OriginalCellIdArrayName.c_str()));
    if (!typedCellIds)
    {
      vtkErrorWithObjectMacro(nullptr, "Pass cell ids array has wrong type.");
    }
    typedCellIds->InsertValue(outputCellIndex, cellId);
  }
}

//----------------------------------------------------------------------------------------------
bool vtkHyperTreeGridGeometryImpl::IsMaskedOrGhost(vtkIdType globalNodeId) const
{
  // This method determines if the globalNodeId offset cell is masked or ghosted.
  return ((this->InMaskArray && this->InMaskArray->GetTuple1(globalNodeId))
      ? true
      : (this->InGhostArray && this->InGhostArray->GetTuple1(globalNodeId)));
}

//----------------------------------------------------------------------------------------------
bool vtkHyperTreeGridGeometryImpl::ProbeForCellInterface(vtkIdType cellId, bool invert)
{
  // This method determines if the cell with the cellId offset cell is a mixed cell
  // and if so, its characteristics.
  // Return :
  // - if there is an interface on this cell (m_hasInterfaceOnThisCell);
  // - the type of the mixed cell (m_cell_interface_type):
  //   - 2 is pure cell;
  //   - -1 is mixed cell with an interface plane describes by m_cell_intercepts[0]; normals is
  //     entering;
  //   - 0 is mixed cell with the double interfaces plane describe by m_cell_intercepts[0]
  //     and m_cell_intercepts[1];
  //   - 1 is mixed cell with an interface describes plane by m_cell_intercepts[1]; normals is
  //     outgoing;
  // - the normals not zero; this same normals for all interfaces plane in the same mixed cell.
  if (!this->HasInterface)
  {
    this->HasInterfaceOnThisCell = false;
    this->CellInterfaceType = 2; // we consider pure cell
    return false;
  }
  double* intercepts = this->InIntercepts->GetTuple(cellId);
  if (intercepts == nullptr)
  {
    this->HasInterfaceOnThisCell = false;
    this->CellInterfaceType = 2; // we consider pure cell
    return false;
  }
  this->CellIntercepts[0] = intercepts[0];
  this->CellIntercepts[1] = intercepts[1];
  this->CellIntercepts[2] = intercepts[2];
  this->CellInterfaceType = static_cast<int>(this->CellIntercepts[2]);
  if (this->CellInterfaceType >= 2)
  {
    this->HasInterfaceOnThisCell = false;
    this->CellInterfaceType = 2; // we consider pure cell
    return false;
  }
  double* normal = this->InNormals->GetTuple(cellId);
  if (normal == nullptr)
  {
    this->HasInterfaceOnThisCell = false;
    this->CellInterfaceType = 2; // we consider pure cell
    return false;
  }
  if (normal[0] == 0. && normal[1] == 0. && normal[2] == 0.)
  {
    this->HasInterfaceOnThisCell = false;
    this->CellInterfaceType = 2; // we consider pure cell
    return false;
  }
  this->CellNormals[0] = normal[0];
  this->CellNormals[1] = normal[1];
  this->CellNormals[2] = normal[2];
  if (this->CellInterfaceType == 0)
  {
    double dD = this->CellIntercepts[1] - this->CellIntercepts[0];
    if (!invert || dD >= 0)
    {
    }
    else
    {
      // In the case of the "sandwich" material defined by two interfaces planes,
      // the implementation considers that :
      // - all interface planes are described by the same normal (u,v,w) ;
      // - an interface plane is described by the equation : u.x+v.y+w.z+d=0 ;
      // - in the direction of the normal, we first traverse the first interface plane
      //   defined by d1 (m_cell_intercepts[0]) then the second interface plane defined
      //   by d2 (m_cell_intercepts[1]).
      // It seems that sometimes the code makes a mistake in the attribution to
      // d1 and d2 which has the effect of disturbing the proper functioning of
      // the implementation.
      // This is why if d2 - d1 is negative, the assignment is reversed.
      // The demonstration of this is easy to achieve starting from the straight line
      // equation of each of the interfaces and the parametric equation of the straight
      // line starting from a point of the interface A towards the interface B.
      // The scalar product of BA by the normal is positive only if d2-d1 is.
      vtkWarningWithObjectMacro(nullptr, "ProbeForCellInterface : d2 - d1 is negative (inverted)");
      double d = this->CellIntercepts[1];
      this->CellIntercepts[1] = this->CellIntercepts[0];
      this->CellIntercepts[0] = d;
    }
  }
  this->HasInterfaceOnThisCell = true;
  return true;
}

//----------------------------------------------------------------------------------------------
double vtkHyperTreeGridGeometryImpl::ComputeDistanceToInterfaceA(const double* xyz) const
{
  // Compute the value of the distance from a point to the interface plane A.
  double val = this->CellIntercepts[0] + this->CellNormals[0] * xyz[0] +
    this->CellNormals[1] * xyz[1] + this->CellNormals[2] * xyz[2];
  return val;
}

//----------------------------------------------------------------------------------------------
double vtkHyperTreeGridGeometryImpl::ComputeDistanceToInterfaceB(const double* xyz) const
{
  // Compute the value of the distance from a point to the interface plane B.
  double val = this->CellIntercepts[1] + this->CellNormals[0] * xyz[0] +
    this->CellNormals[1] * xyz[1] + this->CellNormals[2] * xyz[2];
  return val;
}

VTK_ABI_NAMESPACE_END
