// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkIOSSCellGridReaderInternal.h"

#include "vtkIOSSCellGridReader.h"
#include "vtkIOSSCellGridUtilities.h"
#include "vtkIOSSReaderCommunication.h"
#include "vtkIOSSUtilities.h"

#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkCellMetadata.h"
#include "vtkDGCell.h"
#include "vtkDataArraySelection.h"
#include "vtkDataAssembly.h"
#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"

#include <iostream>

// Uncomment the line below for debugging. You will also need to
// add VTK::IOCellGrid to the IO/IOSS/vtk.module file.
// #define VTK_DBG_IOSS
#ifdef VTK_DBG_IOSS
#include "vtkCellGridWriter.h"
#endif

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

vtkIOSSCellGridReaderInternal::vtkIOSSCellGridReaderInternal(vtkIOSSCellGridReader* self)
  : vtkIOSSReaderInternal(self)
{
}

std::vector<vtkSmartPointer<vtkCellGrid>> vtkIOSSCellGridReaderInternal::GetCellGrids(
  const std::string& blockName, vtkIOSSReader::EntityType vtk_entity_type,
  const DatabaseHandle& handle, int timestep, vtkIOSSCellGridReader* self)
{
  std::string badType;
  auto ioss_entity_type = vtkIOSSUtilities::GetIOSSEntityType(vtk_entity_type);
  switch (ioss_entity_type)
  {
    case Ioss::EntityType::SIDESET:
      // case Ioss::EntityType::SURFACE: // This is a duplicate of SIDESET.
      return this->GetSideSet(blockName, vtk_entity_type, handle, timestep, self);
    case Ioss::EntityType::ELEMENTBLOCK:
      return this->GetElementBlock(blockName, vtk_entity_type, handle, timestep, self);
    case Ioss::EntityType::NODEBLOCK:
      badType = "node block";
      break;
    case Ioss::EntityType::EDGEBLOCK:
      badType = "edge block";
      break;
    case Ioss::EntityType::FACEBLOCK:
      badType = "face block";
      break;
    case Ioss::EntityType::NODESET:
      return this->GetNodeSet(blockName, vtk_entity_type, handle, timestep, self);
    case Ioss::EntityType::EDGESET:
      badType = "edge set";
      break;
    case Ioss::EntityType::FACESET:
      badType = "face set";
      break;
    case Ioss::EntityType::ELEMENTSET:
      badType = "element set";
      break;
    case Ioss::EntityType::COMMSET:
      badType = "comm set";
      break;
    case Ioss::EntityType::SIDEBLOCK:
      badType = "side block";
      break;
    case Ioss::EntityType::REGION:
      badType = "region";
      break;
    case Ioss::EntityType::SUPERELEMENT:
      badType = "superelement";
      break;
    case Ioss::EntityType::STRUCTUREDBLOCK:
      badType = "structured block";
      break;
    case Ioss::EntityType::ASSEMBLY:
      badType = "assembly";
      break;
    case Ioss::EntityType::BLOB:
      badType = "blob";
      break;
    case Ioss::EntityType::INVALID_TYPE:
      badType = "invalid";
      break;
    default:
      badType = "unknown type";
      break;
  }
  throw std::runtime_error("Block " + blockName + " of type " + badType + " unsupported.\n");
}

std::vector<vtkSmartPointer<vtkCellGrid>> vtkIOSSCellGridReaderInternal::GetElementBlock(
  const std::string& blockName, vtkIOSSReader::EntityType vtk_entity_type,
  const DatabaseHandle& handle, int timestep, vtkIOSSCellGridReader* self)
{
  (void)vtk_entity_type;
  auto region = this->GetRegion(handle);
  auto group_entity = region->get_entity(blockName, Ioss::EntityType::ELEMENTBLOCK);
  if (!group_entity)
  {
    throw std::runtime_error("No group entity for element block.");
  }

  int shape_conn_size;
  int shape_order;
  auto grid = vtkSmartPointer<vtkCellGrid>::New();
  auto meta = vtkIOSSCellGridUtilities::GetCellMetadata(
    group_entity, shape_conn_size, shape_order, grid, &this->Cache);
  auto* dg = vtkDGCell::SafeDownCast(meta);
  if (!meta || !dg)
  {
    throw std::runtime_error("Could not read cell specification.");
  }
  if (!vtkIOSSCellGridUtilities::GetConnectivity(
        group_entity, grid, dg, shape_conn_size, &this->Cache))
  {
    throw std::runtime_error("Could not read cell arrays.");
  }
  if (grid->AddCellMetadata(meta) != meta)
  {
    throw std::runtime_error("Cells of this type were already present.");
  }
  // From the shape of cells in the block, the connectivity size, and the order,
  // we need to infer vtkDGCell::CellTypeInfo data (FunctionSpace, Basis, Order).
  auto cellShapeInfo = this->GetCellGridInfoForBlock(shape_conn_size, shape_order, dg);

  // Read node coordinates as the shape attribute.
  // This must always be a "CG" (continuous) attribute.
  vtkIOSSCellGridUtilities::GetShape(
    region, group_entity, cellShapeInfo, timestep, dg, grid, &this->Cache);

#ifdef VTK_DBG_IOSS
  vtkNew<vtkCellGridWriter> wri;
  wri->SetFileName("/tmp/dgDbg.dg");
  wri->SetInputDataObject(grid);
  wri->Write();
#endif
  // Add per-block attributes.
  // auto blockFieldSelection = self->GetFieldSelection(vtk_entity_type);
  // this->GetCellAttributes(blockFieldSelection, grid, dg, region, group_entity, handle, timestep,
  // self->GetReadIds());

  // Add cell-attributes for cell-data and (if not present) point-data.
  auto nodeFieldSelection = self->GetNodeBlockFieldSelection();
  auto nodeblock = region->get_entity("nodeblock_1", Ioss::EntityType::NODEBLOCK);
  this->GetNodalAttributes(nodeFieldSelection, grid->GetAttributes("point-data"_token), grid, dg,
    nodeblock, region, handle, timestep, self->GetReadIds(), "");

  auto elementFieldSelection = self->GetElementBlockFieldSelection();
  this->GetElementAttributes(elementFieldSelection, grid->GetAttributes(dg->GetClassName()), grid,
    dg, group_entity, region, handle, timestep, self->GetReadIds(), "");

#if 0
  // TODO: Support displacements.
  if (self->GetApplyDisplacements())
  {
    this->ApplyDisplacements(dataset, region, group_entity, handle, timestep);
  }
#endif

  return { grid };
}

std::vector<vtkSmartPointer<vtkCellGrid>> vtkIOSSCellGridReaderInternal::GetSideSet(
  const std::string& blockName, vtkIOSSReader::EntityType vtk_entity_type,
  const DatabaseHandle& handle, int timestep, vtkIOSSCellGridReader* self)
{
  (void)self;
  (void)timestep;
  (void)vtk_entity_type;
  std::vector<vtkSmartPointer<vtkCellGrid>> data;
  auto region = this->GetRegion(handle);
  auto group_entity = region->get_entity(blockName, Ioss::EntityType::SIDESET);
  if (!group_entity)
  {
    throw std::runtime_error("No group entity for side set.");
  }
  assert(group_entity->get_database()->get_surface_split_type() == Ioss::SPLIT_BY_ELEMENT_BLOCK);
  auto sideSet = static_cast<Ioss::SideSet*>(group_entity);
  for (auto sideBlock : sideSet->get_side_blocks())
  {
    auto* elementBlock = sideBlock->parent_element_block();
    if (!elementBlock)
    {
      vtkGenericWarningMacro("No parent block for side block.");
      continue;
    }
    std::cout << "    block " << elementBlock->name() << "\n";
#if 0
    int cell_type = VTK_EMPTY_CELL;
    auto cellarray = vtkIOSSUtilities::GetConnectivity(sideBlock, cell_type, &this->Cache);
    if (cellarray != nullptr && cell_type != VTK_EMPTY_CELL)
    {
      blocks.emplace_back(cell_type, cellarray);
    }
#endif
  }

  std::cerr << "Side-sets (" << blockName << ") are currently unsupported.\n";
  return data;
}

std::vector<vtkSmartPointer<vtkCellGrid>> vtkIOSSCellGridReaderInternal::GetNodeSet(
  const std::string& blockName, vtkIOSSReader::EntityType vtk_entity_type,
  const DatabaseHandle& handle, int timestep, vtkIOSSCellGridReader* self)
{
  (void)self;
  (void)timestep;
  (void)vtk_entity_type;
  std::vector<vtkSmartPointer<vtkCellGrid>> data;
  auto region = this->GetRegion(handle);
  auto group_entity = region->get_entity(blockName, Ioss::EntityType::NODESET);
  if (!group_entity)
  {
    throw std::runtime_error("No group entity for node set.");
  }

  std::cerr << "Node-sets (" << blockName << ") are currently unsupported.\n";
  return data;
}

vtkCellAttribute::CellTypeInfo vtkIOSSCellGridReaderInternal::GetCellGridInfoForBlock(
  int shape_conn_size, int shape_order, vtkDGCell* dg)
{
  vtkCellAttribute::CellTypeInfo cellTypeInfo;
  vtkStringToken cellTypeName = dg->GetClassName();
  cellTypeInfo.Order = shape_order;
  if (shape_order == 0)
  {
    cellTypeInfo.Basis = "C"_token;
    return cellTypeInfo;
  }
  int op1 = shape_order + 1;
  // XXX(c++14)
#if __cplusplus < 201400L
  if (cellTypeName == "vtkDGVert"_token)
  {
    if (shape_order > 0)
    {
      throw std::runtime_error("Vertices may only have constant values.");
    }
    cellTypeInfo.Basis = "C"_token;
  }
  else if (cellTypeName == "vtkDGEdge"_token)
  {
    cellTypeInfo.Basis = "C"_token;
  }
  else if (cellTypeName == "vtkDGQuad"_token)
  {
    cellTypeInfo.Basis = (op1 * op1 == shape_conn_size ? "C"_token : "I"_token);
  }
  else if (cellTypeName == "vtkDGTri"_token)
  {
    cellTypeInfo.Basis = "C"_token;
  }
  else if (cellTypeName == "vtkDGPyr"_token)
  {
    switch (shape_conn_size)
    {
      case 13:
        cellTypeInfo.Basis = "I"_token;
        break;
      case 5:
      case 18:
        cellTypeInfo.Basis = "C"_token;
        break;
      case 19:
        cellTypeInfo.Basis = "F"_token;
        break;
      default:
        throw std::runtime_error("Unhandled pyramid connectivity size.");
    }
  }
  else if (cellTypeName == "vtkDGWdg"_token)
  {
    switch (shape_conn_size)
    {
      case 15:
        cellTypeInfo.Basis = "I"_token;
        break;
      case 6:
      case 18:
        cellTypeInfo.Basis = "C"_token;
        break;
      case 21:
        cellTypeInfo.Basis = "F"_token;
        break;
      default:
        throw std::runtime_error("Unhandled wedge connectivity size.");
    }
  }
  else if (cellTypeName == "vtkDGHex"_token)
  {
    switch (shape_conn_size)
    {
      case 20:
        cellTypeInfo.Basis = "I"_token;
        break;
      case 8:
      case 27:
        cellTypeInfo.Basis = "C"_token;
        break;
      default:
        throw std::runtime_error("Unhandled hex connectivity size.");
    }
  }
  else if (cellTypeName == "vtkDGTet"_token)
  {
    switch (shape_conn_size)
    {
      case 4:
      case 10:
        cellTypeInfo.Basis = "C"_token;
        break;
      case 15:
        cellTypeInfo.Basis = "F"_token;
        break;
      default:
        throw std::runtime_error("Unhandled tetrahedron connectivity size.");
    }
  }
  else
  {
    std::string errMsg = "Unhandled cell shape ";
    errMsg += dg->GetClassName();
    errMsg += ".";
    throw std::runtime_error(errMsg);
  }
#else
  switch (cellTypeName.GetId())
  {
    case "vtkDGVert"_hash:
      if (shape_order > 0)
      {
        throw std::runtime_error("Vertices may only have constant values.");
      }
      cellTypeInfo.Basis = "C"_token;
      break;
    case "vtkDGEdge"_hash:
      cellTypeInfo.Basis = "C"_token;
      break;
    case "vtkDGQuad"_hash:
      cellTypeInfo.Basis = (op1 * op1 == shape_conn_size ? "C"_token : "I"_token);
      break;
    case "vtkDGTri"_hash:
      cellTypeInfo.Basis = "C"_token;
      break;
    case "vtkDGPyr"_hash:
      switch (shape_conn_size)
      {
        case 13:
          cellTypeInfo.Basis = "I"_token;
          break;
        case 5:
        case 18:
          cellTypeInfo.Basis = "C"_token;
          break;
        case 19:
          cellTypeInfo.Basis = "F"_token;
          break;
        default:
          throw std::runtime_error("Unhandled pyramid connectivity size.");
      }
      break;
    case "vtkDGWdg"_hash:
      switch (shape_conn_size)
      {
        case 15:
          cellTypeInfo.Basis = "I"_token;
          break;
        case 6:
        case 18:
          cellTypeInfo.Basis = "C"_token;
          break;
        case 21:
          cellTypeInfo.Basis = "F"_token;
          break;
        default:
          throw std::runtime_error("Unhandled wedge connectivity size.");
      }
      break;
    case "vtkDGHex"_hash:
      switch (shape_conn_size)
      {
        case 20:
          cellTypeInfo.Basis = "I"_token;
          break;
        case 8:
        case 27:
          cellTypeInfo.Basis = "C"_token;
          break;
        default:
          throw std::runtime_error("Unhandled hex connectivity size.");
      }
      break;
    case "vtkDGTet"_hash:
      switch (shape_conn_size)
      {
        case 4:
        case 10:
          cellTypeInfo.Basis = "C"_token;
          break;
        case 15:
          cellTypeInfo.Basis = "F"_token;
          break;
        default:
          throw std::runtime_error("Unhandled tetrahedron connectivity size.");
      }
      break;
    default:
    {
      std::string errMsg = "Unhandled cell shape ";
      errMsg += dg->GetClassName();
      errMsg += ".";
      throw std::runtime_error(errMsg);
    }
  }
#endif
  return cellTypeInfo;
}

void vtkIOSSCellGridReaderInternal::GetNodalAttributes(vtkDataArraySelection* fieldSelection,
  vtkDataSetAttributes* arrayGroup, vtkCellGrid* grid, vtkDGCell* meta,
  Ioss::GroupingEntity* group_entity, Ioss::Region* region, const DatabaseHandle& handle,
  int timestep, bool read_ioss_ids, const std::string& cache_key_suffix)
{
  vtkIdTypeArray* ids_to_extract = nullptr;
  std::vector<std::string> fieldnames;
  std::string globalIdsFieldName;
  if (read_ioss_ids)
  {
    switch (group_entity->type())
    {
      case Ioss::EntityType::NODEBLOCK:
      case Ioss::EntityType::EDGEBLOCK:
      case Ioss::EntityType::FACEBLOCK:
      case Ioss::EntityType::ELEMENTBLOCK:
        fieldnames.emplace_back("ids");
        globalIdsFieldName = "ids";
        break;

      case Ioss::EntityType::NODESET:
        break;

      case Ioss::EntityType::STRUCTUREDBLOCK:
        // Unsupported.
        break;

      case Ioss::EntityType::EDGESET:
      case Ioss::EntityType::FACESET:
      case Ioss::EntityType::ELEMENTSET:
      case Ioss::EntityType::SIDESET:
        fieldnames.emplace_back("element_side");
        break;

      default:
        break;
    }
  }
  for (int cc = 0; fieldSelection != nullptr && cc < fieldSelection->GetNumberOfArrays(); ++cc)
  {
    if (fieldSelection->GetArraySetting(cc))
    {
      fieldnames.emplace_back(fieldSelection->GetArrayName(cc));
    }
  }
  auto shapeInfo = grid->GetShapeAttribute()->GetCellTypeInfo(meta->GetClassName());
  for (const auto& fieldname : fieldnames)
  {
    if (auto array = this->GetField(
          fieldname, region, group_entity, handle, timestep, ids_to_extract, cache_key_suffix))
    {
      if (fieldname == globalIdsFieldName)
      {
        arrayGroup->SetGlobalIds(vtkDataArray::SafeDownCast(array));
      }
      else if (fieldname == vtkDataSetAttributes::GhostArrayName())
      {
        // Handle vtkGhostType attribute specially. Convert it to the expected vtkUnsignedCharArray.
        vtkNew<vtkUnsignedCharArray> ghostArray;
        ghostArray->SetName(vtkDataSetAttributes::GhostArrayName());
        ghostArray->SetNumberOfComponents(1);
        ghostArray->SetNumberOfTuples(array->GetNumberOfTuples());

        ghostArray->CopyComponent(0, vtkDataArray::SafeDownCast(array), 0);
        arrayGroup->AddArray(ghostArray);
        array = ghostArray;
      }
      else
      {
        arrayGroup->AddArray(array);
      }
      // TODO: If the attribute and shape-function do not have the same order, the
      //       connectivity array must be different. This is not supported yet by the
      //       Exodus standard AFAIK.
      vtkNew<vtkCellAttribute> attribute;
      attribute->Initialize(array->GetName(), "ℝ³", array->GetNumberOfComponents());
      vtkCellAttribute::CellTypeInfo cellTypeInfo;
      cellTypeInfo.DOFSharing = "point-data"_token;
      cellTypeInfo.FunctionSpace = "HGRAD"; // All point-data arrays are HGRAD
      cellTypeInfo.Basis = shapeInfo.Basis;
      cellTypeInfo.Order = shapeInfo.Order;
      cellTypeInfo.ArraysByRole["connectivity"] = meta->GetCellSpec().Connectivity;
      cellTypeInfo.ArraysByRole["values"] = array;
      attribute->SetCellTypeInfo(meta->GetClassName(), cellTypeInfo);
      grid->AddCellAttribute(attribute);
    }
  }
}

void vtkIOSSCellGridReaderInternal::GetElementAttributes(vtkDataArraySelection* fieldSelection,
  vtkDataSetAttributes* arrayGroup, vtkCellGrid* grid, vtkDGCell* meta,
  Ioss::GroupingEntity* group_entity, Ioss::Region* region, const DatabaseHandle& handle,
  int timestep, bool read_ioss_ids, const std::string& cache_key_suffix)
{
  vtkIdTypeArray* ids_to_extract = nullptr;
  std::vector<std::string> fieldnames;
  std::string globalIdsFieldName;
  if (read_ioss_ids)
  {
    switch (group_entity->type())
    {
      case Ioss::EntityType::NODEBLOCK:
      case Ioss::EntityType::EDGEBLOCK:
      case Ioss::EntityType::FACEBLOCK:
      case Ioss::EntityType::ELEMENTBLOCK:
        fieldnames.emplace_back("ids");
        globalIdsFieldName = "ids";
        break;

      case Ioss::EntityType::NODESET:
        break;

      case Ioss::EntityType::STRUCTUREDBLOCK:
        // Unsupported.
        break;

      case Ioss::EntityType::EDGESET:
      case Ioss::EntityType::FACESET:
      case Ioss::EntityType::ELEMENTSET:
      case Ioss::EntityType::SIDESET:
        fieldnames.emplace_back("element_side");
        break;

      default:
        break;
    }
  }
  for (int cc = 0; fieldSelection != nullptr && cc < fieldSelection->GetNumberOfArrays(); ++cc)
  {
    if (fieldSelection->GetArraySetting(cc))
    {
      fieldnames.emplace_back(fieldSelection->GetArrayName(cc));
    }
  }
  auto shapeInfo = grid->GetShapeAttribute()->GetCellTypeInfo(meta->GetClassName());
  for (const auto& fieldname : fieldnames)
  {
    if (auto array = this->GetField(
          fieldname, region, group_entity, handle, timestep, ids_to_extract, cache_key_suffix))
    {
      if (fieldname == globalIdsFieldName)
      {
        arrayGroup->SetGlobalIds(vtkDataArray::SafeDownCast(array));
      }
      else if (fieldname == vtkDataSetAttributes::GhostArrayName())
      {
        // Handle vtkGhostType attribute specially. Convert it to the expected vtkUnsignedCharArray.
        vtkNew<vtkUnsignedCharArray> ghostArray;
        ghostArray->SetName(vtkDataSetAttributes::GhostArrayName());
        ghostArray->SetNumberOfComponents(1);
        ghostArray->SetNumberOfTuples(array->GetNumberOfTuples());

        ghostArray->CopyComponent(0, vtkDataArray::SafeDownCast(array), 0);
        arrayGroup->AddArray(ghostArray);
        array = ghostArray;
      }
      else
      {
        arrayGroup->AddArray(array);
      }
      // TODO: If the attribute and shape-function do not have the same order, the
      //       connectivity array must be different. This is not supported yet by the
      //       Exodus standard AFAIK.
      vtkNew<vtkCellAttribute> attribute;
      vtkCellAttribute::CellTypeInfo cellTypeInfo;
      // cellTypeInfo.DOFSharing = cellTypeName;

      // TODO: FIXME: URHERE: This is a hack. We should inspect the "info" records
      //       and glom fields according to them rather than assuming that if the
      //       field has numEdges components it is DG HCURL, numFaces components it is
      //       DG HDIV, and numPoints components it is DG HGRAD (else CG HGRAD).
      if (array->GetNumberOfComponents() == meta->GetNumberOfSidesOfDimension(1))
      {
        attribute->Initialize(array->GetName(), "ℝ³", 3);
        cellTypeInfo.FunctionSpace = "HCURL"; // TODO
        cellTypeInfo.Basis = "I";             // TODO
        cellTypeInfo.Order = 1;               // TODO
      }
      else if (array->GetNumberOfComponents() ==
        meta->GetNumberOfSidesOfDimension(meta->GetDimension() - 1))
      {
        attribute->Initialize(array->GetName(), "ℝ³", 3);
        cellTypeInfo.FunctionSpace = "HDIV"; // TODO
        cellTypeInfo.Basis = "I";            // TODO
        cellTypeInfo.Order = 1;              // TODO
      }
      else
      {
        if (array->GetNumberOfComponents() == meta->GetNumberOfSidesOfDimension(0))
        {
          attribute->Initialize(array->GetName(), "ℝ³", 1);
          cellTypeInfo.FunctionSpace = "HGRAD";
          cellTypeInfo.Basis = shapeInfo.Basis;
          cellTypeInfo.Order = shapeInfo.Order;
        }
        else
        {
          attribute->Initialize(array->GetName(), "ℝ³", array->GetNumberOfComponents());
          cellTypeInfo.FunctionSpace = "constant"; // TODO
          cellTypeInfo.Basis = "C";                // TODO
          cellTypeInfo.Order = 0;                  // TODO
        }
      }
      // cellTypeInfo.ArraysByRole["connectivity"] = meta->GetCellSpec().Connectivity;
#if 0
      // This is to see whether single-precision floats are causing the rendering issues.
      double* ar = vtkDataArray::SafeDownCast(array)->GetRange(-1);
      double arbd = std::max(std::abs(ar[0]), std::abs(ar[1]));
      if (arbd == 0) { arbd = 1e-8; }
      if (arbd < 1e-7)
      {
        double scale = 1. / arbd;
        vtkNew<vtkDoubleArray> scaled;
        scaled->DeepCopy(array);
        vtkIdType nn = scaled->GetMaxId();
        double* vv = scaled->GetPointer(0);
        for (vtkIdType ii = 0; ii < nn; ++ii)
        {
          vv[ii] = vv[ii] * scale;
        }
        arrayGroup->RemoveArray(array->GetName());
        arrayGroup->AddArray(scaled);
        cellTypeInfo.ArraysByRole["values"] = scaled;
        attribute->SetCellTypeInfo(meta->GetClassName(), cellTypeInfo);
      }
      else
#endif
      {
        cellTypeInfo.ArraysByRole["values"] = array;
        attribute->SetCellTypeInfo(meta->GetClassName(), cellTypeInfo);
      }
      grid->AddCellAttribute(attribute);
    }
  }
}

VTK_ABI_NAMESPACE_END
