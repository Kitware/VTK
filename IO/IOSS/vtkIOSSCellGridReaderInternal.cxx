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
#include "vtkSMPTools.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVector.h"

#include <iostream>

// Uncomment the line below for debugging. You will also need to
// add VTK::IOCellGrid to the IO/IOSS/vtk.module file.
// #define VTK_DBG_IOSS
#ifdef VTK_DBG_IOSS
#include "vtkCellGridWriter.h"
#endif

// clang-format off
#include VTK_IOSS(Ioss_Field.h)
#include VTK_IOSS(Ioss_TransformFactory.h)
// clang-format on

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
      // badType = "edge block";
      return {}; // Ignore edge blocks; they are only used to read HCurl fields.
    case Ioss::EntityType::FACEBLOCK:
      // badType = "face block";
      return {}; // Ignore face blocks; they are only used to read HDiv fields.
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

  return this->GetElementBlock(blockName, group_entity, handle, timestep, self);
}

std::vector<vtkSmartPointer<vtkCellGrid>> vtkIOSSCellGridReaderInternal::GetElementBlock(
  const std::string& blockName, const Ioss::GroupingEntity* group_entity,
  const DatabaseHandle& handle, int timestep, vtkIOSSCellGridReader* self)
{
  (void)blockName;

  auto region = this->GetRegion(handle);
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
        group_entity, grid, dg, shape_conn_size, -1, std::string(), &this->Cache))
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
  // Apply displacements before reading other cell-attributes as
  // computing the range of HDIV/HCURL attributes **must** use
  // the actual (deformed) cell shape. Also, note that using a
  // displacement scale factor other than 1.0 will introduce errors.
  if (self->GetApplyDisplacements())
  {
    this->ApplyDisplacements(grid, region, group_entity, handle, timestep);
  }

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

#ifdef VTK_DBG_IOSS
  vtkNew<vtkCellGridWriter> wri;
  std::ostringstream dbgName;
  dbgName << "/tmp/dbg_ioss_" << blockName << ".dg";
  wri->SetFileName(dbgName.str().c_str());
  wri->SetInputDataObject(0, grid);
  wri->Write();
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
    const auto* elementBlock = sideBlock->parent_element_block();
    if (!elementBlock)
    {
      vtkGenericWarningMacro("No parent block for side block.");
      continue;
    }
#ifdef VTK_DBG_IOSS
    std::cout << "   side set " << group_entity->name() << " side block " << sideBlock->name()
              << " parent element block " << elementBlock->name() << "\n";
#endif
    // There really should be only a single cell-grid returned for any element block,
    // but sources is a vector. For now, fail hard if sources holds more than 1 cell-grid.
    auto sources =
      this->GetElementBlock(elementBlock->name(), elementBlock, handle, timestep, self);
    if (sources.size() != 1)
    {
      throw std::logic_error("Side block " + sideBlock->name() + " of side set " +
        group_entity->name() + " with parent " + elementBlock->name() + " has " +
        std::to_string(sources.size()) + " cell-grids, but 1 is expected.");
    }
    // Now that we've ensured "sources" matches our expectation, get the lone vtkCellGrid from it
    // and the lone cell-metadata entry in it:
    auto& eblk(sources.front());
    auto* dg = vtkDGCell::SafeDownCast(eblk->GetCellType(eblk->GetCellTypes().front()));

    auto side_raw = sideBlock->get_field("element_side_raw");
    auto sideConn = vtkIOSSUtilities::CreateArray(side_raw);
    if (side_raw.zero_copy_enabled())
    {
      void* values;
      size_t values_size;
      sideBlock->get_field_data("element_side_raw", &values, &values_size);
      sideConn->SetVoidArray(values, static_cast<vtkIdType>(values_size), 1);
    }
    else
    {
      sideBlock->get_field_data("element_side_raw", sideConn->GetVoidPointer(0),
        sideConn->GetDataSize() * sideConn->GetDataTypeSize());
    }
#ifdef VTK_DBG_IOSS
    std::cout << "side field "
              << " " << sideConn->GetNumberOfTuples() << "×" << sideConn->GetNumberOfComponents()
              << " [" << sideConn->GetRange(0)[0] << ", " << sideConn->GetRange(0)[1] << "]"
              << " [" << sideConn->GetRange(1)[0] << ", " << sideConn->GetRange(1)[1] << "]"
              << "\n";
#endif

    // Transform sideConn to match VTK's data model:
    // 1. The cell IDs (file-local element IDs in ioss parlance) must be offset by the
    //    starting ID of the parent elementBlock.
    // 2. The side IDs (Exodus side numbering) must be transformed to match the
    //    side numbering of the vtkDGCell subclasses.
    // The latter also enables us to detect the side-spec SideType and SourceShape.
    // vtkIOSSCellGridUtilities::AdjustSideConnectivity(eblk, dg, sideConn, sideBlock,
    // elementBlock);

    dg->GetCellSpec().Blanked = true; // Blank the parent cell-spec
    dg->GetSideSpecs().resize(1);     // Add a child side-spec.
    auto& sideSpec(dg->GetCellSource(0));
    sideSpec.Connectivity = sideConn;
    sideSpec.Offset = 0; // True for Exodus files, since we blanked the cell-spec.
    // Now we need to determine the dimension of the sides.
    // To do this, we will assume (and for historical reasons, this has always been
    // how Exodus is used) that sides in a side set are all of the same dimension
    // and (for a given sideBlock) of the same shape.

    // vtkIOSSCellGridUtilities::GetConnectivity(sideBlock, eblk, dg, 2, 0, "sides", &this->Cache);
    auto* sideArray = vtkTypeInt32Array::SafeDownCast(dg->GetCellSource(0).Connectivity);
    auto cellIdOffset = elementBlock->get_offset() + /* switch from 1- to 0-based indexing*/ 1;
    std::map<int, int> permutations;
    int firstSideIdx = -1;
    vtkSMPTools::For(0, sideArray->GetNumberOfTuples(),
      [sideArray, cellIdOffset, &firstSideIdx, &permutations](vtkIdType begin, vtkIdType end)
      {
        std::array<vtkTypeUInt64, 2> sideTuple;
        for (vtkIdType mm = begin; mm < end; ++mm)
        {
          sideArray->GetUnsignedTuple(mm, sideTuple.data());
          sideTuple[0] -= cellIdOffset;
          sideTuple[1] = permutations.empty() ? sideTuple[1] - 1
                                              : // switch from 1- to 0-based indexing.
            permutations[sideTuple[1]];
          sideArray->SetUnsignedTuple(mm, sideTuple.data());
        }
        if (begin == 0)
        {
          // Only write to firstSideIdx if we are asked to process
          // the first tuple.
          sideArray->GetUnsignedTuple(0, sideTuple.data());
          firstSideIdx = permutations.empty() ? sideTuple[1] - 1 : permutations[sideTuple[1]];
        }
      });
    sideSpec.SourceShape = dg->GetSideShape(firstSideIdx);
    sideSpec.SideType = dg->GetSideTypeForShape(sideSpec.SourceShape);
    std::ostringstream arrayGroupName;
    arrayGroupName << vtkDGCell::GetShapeName(sideSpec.SourceShape).Data() << " sides of "
                   << dg->GetClassName();
#ifdef VTK_DBG_IOSS
    std::cout << "   sides stored in group \"" << arrayGroupName.str() << "\".\n";
#endif
    eblk->GetAttributes(arrayGroupName.str())->AddArray(sideConn);
    data.push_back(eblk);
  }

  return data;
}

std::vector<vtkSmartPointer<vtkCellGrid>> vtkIOSSCellGridReaderInternal::GetNodeSet(
  const std::string& blockName, vtkIOSSReader::EntityType vtk_entity_type,
  const DatabaseHandle& handle, int timestep, vtkIOSSCellGridReader* self)
{
  (void)self;
  (void)vtk_entity_type;
  auto region = this->GetRegion(handle);
  auto group_entity = region->get_entity(blockName, Ioss::EntityType::NODESET);
  if (!group_entity)
  {
    vtkErrorWithObjectMacro(self, "No group entity for node-set \"" << blockName << "\".");
    return {};
  }
  auto grid = vtkSmartPointer<vtkCellGrid>::New();
  auto meta = vtkCellMetadata::NewInstance("vtkDGVert"_token, grid);
  auto* dg = vtkDGCell::SafeDownCast(meta);
  if (!meta || !dg)
  {
    vtkErrorWithObjectMacro(
      self, "Could not create metadata for node-set \"" << blockName << "\".");
    return {};
  }
  if (!grid->AddCellMetadata(meta))
  {
    vtkErrorWithObjectMacro(
      self, "Could not add metadata for node-set \"" << blockName << "\" to grid.");
    return {};
  }
  // Fetch the IDs of the file-global points included in the node-set,
  // offsetting by -1 so they are 0-indexed:
  auto transform = std::unique_ptr<Ioss::Transform>(Ioss::TransformFactory::create("offset"));
  transform->set_property("offset", -1);
  auto ids_raw = vtkIOSSUtilities::GetData(group_entity, "ids_raw", transform.get());
  ids_raw->SetNumberOfComponents(1);

  // Add the ID array to a vtkDataSetAttributes instance corresponding to
  // the number of cells of the node-set. Since a separate cell-grid holds
  // each node-set, we use the name of the cell type ("vtkDGVert") for the
  // array group:
  auto* cellGroup = grid->GetAttributes(dg->GetClassName());
  cellGroup->AddArray(ids_raw);
  dg->GetCellSpec().Connectivity = ids_raw;
  dg->GetCellSpec().SourceShape = vtkDGCell::Shape::Vertex;
  dg->GetCellSpec().Blanked = false;

  // From the shape of cells in the block, the connectivity size, and the order,
  // we need to infer vtkDGCell::CellTypeInfo data (FunctionSpace, Basis, Order).
  vtkCellAttribute::CellTypeInfo cellShapeInfo;
  cellShapeInfo.FunctionSpace = "constant"_token;
  cellShapeInfo.Basis = "C"_token;
  cellShapeInfo.Order = 0;

  // Read node coordinates as the shape attribute.
  // This must always be a "CG" (continuous) attribute.
  vtkIOSSCellGridUtilities::GetShape(
    region, group_entity, cellShapeInfo, timestep, dg, grid, &this->Cache);
  // Apply displacements before reading other cell-attributes as
  // computing the range of HDIV/HCURL attributes **must** use
  // the actual (deformed) cell shape. Also, note that using a
  // displacement scale factor other than 1.0 will introduce errors.
  if (self->GetApplyDisplacements())
  {
    this->ApplyDisplacements(grid, region, group_entity, handle, timestep);
  }

  // TODO: Add per-block attributes.
  // Note that this is very difficult since nodes may be attached to
  // cells in any number (0 or more) of element blocks. We could add
  // all cell-attributes across all element blocks (assuming there are
  // no cell-attributes with different spaces/components in different
  // blocks), but then we must have NaN entries for nodes that do not
  // attach to cells in those blocks. Also, it is possible for a node
  // to take on multiple values if multiple element blocks provide the
  // same field. In that case, it is unclear what to do.

  // Add cell-attributes for nodal-data.
  auto nodeFieldSelection = self->GetNodeBlockFieldSelection();
  auto nodeblock = region->get_entity("nodeblock_1", Ioss::EntityType::NODEBLOCK);
  this->GetNodalAttributes(nodeFieldSelection, grid->GetAttributes("point-data"_token), grid, dg,
    nodeblock, region, handle, timestep, self->GetReadIds(), "");

  return { grid };
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
  return cellTypeInfo;
}

void vtkIOSSCellGridReaderInternal::GetNodalAttributes(vtkDataArraySelection* fieldSelection,
  vtkDataSetAttributes* arrayGroup, vtkCellGrid* grid, vtkDGCell* meta,
  const Ioss::GroupingEntity* group_entity, Ioss::Region* region, const DatabaseHandle& handle,
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
      // Point-data arrays must match the shape-attribute since they are
      // continuous and must thus use the connectivity array provided for
      // the shape attribute. Note that even nodesets and blocks of vertex
      // cells are "continuous," though in that case they live in the
      // "constant" function space, not "HGRAD."
      cellTypeInfo.FunctionSpace = shapeInfo.FunctionSpace;
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
  const Ioss::GroupingEntity* group_entity, Ioss::Region* region, const DatabaseHandle& handle,
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

bool vtkIOSSCellGridReaderInternal::ApplyDisplacements(vtkCellGrid* grid, Ioss::Region* region,
  const Ioss::GroupingEntity* group_entity, const DatabaseHandle& handle, int timestep)
{
  if (!group_entity)
  {
    return false;
  }

  if (group_entity->type() == Ioss::EntityType::STRUCTUREDBLOCK)
  {
    // CGNS
    vtkErrorWithObjectMacro(grid, "CGNS is unsupported.");
    return false;
  }

  // We rely on the exodus conventions that (1) points are global across
  // all blocks; and (2) each grid holds a single type of cell.
  auto cellTypes = grid->CellTypeArray();
  if (cellTypes.empty())
  {
    vtkWarningWithObjectMacro(grid, "Exodus grid has no cells; thus no points to displace.");
    return false;
  }
  auto shapeAtt = grid->GetShapeAttribute();
  auto shapeInfo = shapeAtt->GetCellTypeInfo(cellTypes.front());
  auto* coords = vtkDataArray::SafeDownCast(shapeInfo.ArraysByRole["values"_token]);

  // For now, we only support exodus-formatted data (which has a single block of point coordinates).
  // So we can look the cache up based on the node_block:
  auto node_block = region->get_entity("nodeblock_1", Ioss::EntityType::NODEBLOCK);
  auto& cache = this->Cache;
  const auto xformPtsCacheKeyEnding =
    std::to_string(timestep) + std::to_string(std::hash<double>{}(this->DisplacementMagnitude));
  const auto xformPtsCacheKey = "__vtk_xformed_pts_" + xformPtsCacheKeyEnding;
  if (auto* xformedPts = vtkDataArray::SafeDownCast(cache.Find(node_block, xformPtsCacheKey)))
  {
    auto* pointGroup = grid->GetAttributes("coordinates"_token);
    assert(xformedPts->GetNumberOfTuples() == pointGroup->GetNumberOfTuples());
    // Remove the undeflected points:
    pointGroup->RemoveArray(coords->GetName());
    // Add the deflected points:
    pointGroup->SetScalars(xformedPts);
    for (const auto& cellTypeToken : cellTypes)
    {
      shapeInfo = shapeAtt->GetCellTypeInfo(cellTypeToken);
      shapeInfo.ArraysByRole["values"_token] = xformedPts;
      if (!shapeAtt->SetCellTypeInfo(cellTypeToken, shapeInfo))
      {
        vtkErrorWithObjectMacro(grid,
          "Failed to update cell-type info for " << cellTypeToken.Data() << " on "
                                                 << shapeAtt->GetName().Data() << ".");
      }
    }
    return true;
  }

  vtkSmartPointer<vtkDataArray> array;

  auto displ_array_name = vtkIOSSUtilities::GetDisplacementFieldName(node_block);
  if (displ_array_name.empty())
  {
    // NB: This is not an error; it may be that the simulation simply doesn't deform the mesh.
    // vtkErrorWithObjectMacro(grid, "No displacement field.");
    return false;
  }
  array = vtkDataArray::SafeDownCast(
    this->GetField(displ_array_name, region, node_block, handle, timestep, nullptr, std::string()));

  if (coords && array)
  {
    vtkIdType npts = coords->GetNumberOfTuples();
    auto* xformedPts = coords->NewInstance();
    xformedPts->SetName(coords->GetName());
    xformedPts->SetNumberOfComponents(3);
    xformedPts->SetNumberOfTuples(npts);
    double scale = this->DisplacementMagnitude;
    vtkSMPTools::For(0, npts,
      [&](vtkIdType begin, vtkIdType end)
      {
        vtkVector3d point{ 0.0 }, displ{ 0.0 };
        for (vtkIdType ii = begin; ii != end; ++ii)
        {
          coords->GetTuple(ii, point.GetData());
          array->GetTuple(ii, displ.GetData());
          for (int jj = 0; jj < 3; ++jj)
          {
            displ[jj] *= scale;
          }
          xformedPts->SetTuple(ii, (point + displ).GetData());
        }
      });
    auto* pointGroup = grid->GetAttributes("coordinates"_token);
    // Remove the undeflected points:
    pointGroup->RemoveArray(coords->GetName());
    // Add the deflected points:
    pointGroup->SetScalars(xformedPts);
    for (const auto& cellTypeToken : cellTypes)
    {
      auto cellShapeInfo = shapeAtt->GetCellTypeInfo(cellTypeToken);
      // auto* coords = cellShapeInfo.ArraysByRole["values"_token];
      cellShapeInfo.ArraysByRole["values"_token] = xformedPts;
      if (!shapeAtt->SetCellTypeInfo(cellTypeToken, cellShapeInfo))
      {
        vtkErrorWithObjectMacro(grid,
          "Failed to update cell-type info for " << cellTypeToken.Data() << " on "
                                                 << shapeAtt->GetName().Data() << ".");
      }
    }
    cache.Insert(node_block, xformPtsCacheKey, xformedPts);
    xformedPts->FastDelete();
    return true;
  }
  return false;
}

VTK_ABI_NAMESPACE_END
