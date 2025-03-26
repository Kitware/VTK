// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPIController.h"
#include "vtk_mpi.h"
#endif

#include "vtkIOSSCellGridUtilities.h"
#include "vtkIOSSUtilities.h"

#include "vtkArrayDispatch.h"
#include "vtkCellGrid.h"
#include "vtkCellMetadata.h"
#include "vtkDGCell.h"
#include "vtkDataSet.h"
#include "vtkIdTypeArray.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"

#include <vtksys/RegularExpression.hxx>
#include <vtksys/SystemTools.hxx>

// Ioss includes
#include <vtk_ioss.h>
// clang-format off
#include VTK_IOSS(Ioss_ElementTopology.h)
#include VTK_IOSS(Ioss_Field.h)
#include VTK_IOSS(Ioss_NodeBlock.h)
#include VTK_IOSS(Ioss_SideBlock.h)
#include VTK_IOSS(Ioss_SideSet.h)
#include VTK_IOSS(Ioss_TransformFactory.h)
// clang-format on

#include <memory>

namespace vtkIOSSCellGridUtilities
{
VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

struct ChangeComponentsImpl
{
  vtkDataArray* Input;
  template <typename ArrayT>
  void operator()(ArrayT* output)
  {
    using ValueType = typename ArrayT::ValueType;
    ArrayT* input = vtkArrayDownCast<ArrayT>(this->Input);
    const int numComps = std::max(input->GetNumberOfComponents(), output->GetNumberOfComponents());
    ValueType* tuple = new ValueType[numComps];
    std::fill(tuple, tuple + numComps, static_cast<ValueType>(0));
    for (vtkIdType cc = 0, max = input->GetNumberOfTuples(); cc < max; ++cc)
    {
      input->GetTypedTuple(cc, tuple);
      output->SetTypedTuple(cc, tuple);
    }
    delete[] tuple;
  }
};

static vtkSmartPointer<vtkDataArray> ChangeComponents(vtkDataArray* array, int num_components)
{
  if (array == nullptr || (array->GetNumberOfComponents() == num_components))
  {
    return array;
  }

  vtkSmartPointer<vtkDataArray> result;
  result.TakeReference(array->NewInstance());
  result->SetName(array->GetName());
  result->SetNumberOfComponents(num_components);
  result->SetNumberOfTuples(array->GetNumberOfTuples());

  ChangeComponentsImpl worker{ array };
  using SupportedArrays = vtkIOSSUtilities::ArrayList;
  using Dispatch = vtkArrayDispatch::DispatchByArray<SupportedArrays>;
  if (!Dispatch::Execute(result, worker))
  {
    throw std::runtime_error("Failed to strip extra components from array!");
  }
  return result;
}

struct Swizzler
{
  const std::vector<int>& Ordering;

  template <typename ArrayT>
  void operator()(ArrayT* array)
  {
    const int numComps = array->GetNumberOfComponents();
    using ValueType = typename ArrayT::ValueType;
    ValueType* inTuple = new ValueType[numComps];
    ValueType* outTuple = new ValueType[numComps];
    for (vtkIdType cc = 0, max = array->GetNumberOfTuples(); cc < max; ++cc)
    {
      array->GetTypedTuple(cc, inTuple);
      for (int comp = 0; comp < numComps; ++comp)
      {
        outTuple[comp] = inTuple[this->Ordering[comp]];
      }
      array->SetTypedTuple(cc, outTuple);
    }
    delete[] inTuple;
    delete[] outTuple;
  }
};

static bool SwizzleComponents(vtkDataArray* array, const std::vector<int>& ordering)
{
  Swizzler worker{ ordering };
  using SupportedArrays = vtkIOSSUtilities::ArrayList;
  using Dispatch = vtkArrayDispatch::DispatchByArray<SupportedArrays>;
  if (!Dispatch::Execute(array, worker))
  {
    throw std::runtime_error("Failed to strip extra components from array!");
  }
  return true;
}

vtkSmartPointer<vtkCellMetadata> GetCellMetadata(const Ioss::ElementTopology* topology,
  int& ioss_cell_points, int& ioss_cell_order, vtkCellGrid* cellGrid)
{
  vtkSmartPointer<vtkCellMetadata> cellType;
  switch (topology->shape())
  {
    case Ioss::ElementShape::SPHERE:
    case Ioss::ElementShape::POINT:
      cellType = vtkCellMetadata::NewInstance("vtkDGVert", cellGrid);
      break;

    case Ioss::ElementShape::SPRING:
    case Ioss::ElementShape::LINE:
      cellType = vtkCellMetadata::NewInstance("vtkDGEdge", cellGrid);
      break;

    case Ioss::ElementShape::TRI:
      cellType = vtkCellMetadata::NewInstance("vtkDGTri", cellGrid);
      break;

    case Ioss::ElementShape::QUAD:
      cellType = vtkCellMetadata::NewInstance("vtkDGQuad", cellGrid);
      break;

    case Ioss::ElementShape::TET:
      cellType = vtkCellMetadata::NewInstance("vtkDGTet", cellGrid);
      break;

    case Ioss::ElementShape::PYRAMID:
      cellType = vtkCellMetadata::NewInstance("vtkDGPyr", cellGrid);
      break;

    case Ioss::ElementShape::WEDGE:
      cellType = vtkCellMetadata::NewInstance("vtkDGWdg", cellGrid);
      break;

    case Ioss::ElementShape::HEX:
      cellType = vtkCellMetadata::NewInstance("vtkDGHex", cellGrid);
      break;

    case Ioss::ElementShape::UNKNOWN:
      // This happens for superelements.
    default:
      break;
  }
  ioss_cell_points = cellType ? topology->number_nodes() : -1;
  ioss_cell_order = cellType ? topology->order() : -1;
#if 0
  if (auto* dgCell = vtkDGCell::SafeDownCast(cellType))
  {
    if (nodesPerCell > 0)
    {
      // Create a new array but do not allocate storage for it.
      auto* conn = vtkIdTypeArray::New();
      conn->SetNumberOfComponents(nodesPerCell);
      dgCell->GetCellSpec().Connectivity = conn;
    }
  }
#endif
  return cellType;
}

const Ioss::ElementTopology* GetElementTopology(vtkCellMetadata* cellType)
{
  (void)cellType;
  // TODO: Implement.
  return nullptr;
}

bool ConnectivityNeedsPermutation(vtkDGCell* meta, int ioss_cell_points,
  // int ioss_cell_order,
  std::vector<int>& permutation)
{
  permutation.clear();
  vtkStringToken cellType = meta->GetClassName();
  // XXX(c++14)
#if __cplusplus < 201400L
  if (cellType == "vtkDGTet"_token)
  {
    if (ioss_cell_points == 15)
    {
      permutation = { // Corner vertices
        0, 1, 2, 3,
        // Edges
        4, 5, 6, 7, 8, 9,
        // Faces
        11, 14, 12, 13,
        // Body-centered
        10
      };
    }
  }
  else if (cellType == "vtkDGWdg"_token)
  {
    if (ioss_cell_points == 21)
    {
      permutation = { /* 2 triangles */
        3, 4, 5, 0, 1, 2,

        /* edge centers */
        12, 13, 14, 6, 7, 8, 9, 10, 11,

        /* triangle centers */
        17, 16,

        /* quad-centers */
        20, 18, 19,

        /* body center */
        15
      };
    }
  }
  else if (cellType == "vtkDGPyr"_token)
  {
    switch (ioss_cell_points)
    {
      case 18:
      case 19:
        permutation = { /* corners */
          2, 3, 0, 1, 4,
          /* mid-edge points */
          7, 8, 5, 6, 11, 12, 9, 10,
          /* mid-face points */
          17, 15, 16, 13, 14
        };
        if (ioss_cell_points == 18)
        {
          break;
        }
        permutation.push_back(18);
        break;
      default:
        break;
    }
  }
#else
  switch (cellType.GetId())
  {
    case "vtkDGTet"_hash:
      if (ioss_cell_points == 15)
      {
        permutation = { // Corner vertices
          0, 1, 2, 3,
          // Edges
          4, 5, 6, 7, 8, 9,
          // Faces
          11, 14, 12, 13,
          // Body-centered
          10
        };
      }
      break;
    case "vtkDGWdg"_hash:
      if (ioss_cell_points == 21)
      {
        permutation = { /* 2 triangles */
          3, 4, 5, 0, 1, 2,

          /* edge centers */
          12, 13, 14, 6, 7, 8, 9, 10, 11,

          /* triangle centers */
          17, 16,

          /* quad-centers */
          20, 18, 19,

          /* body center */
          15
        };
      }
      break;
    case "vtkDGPyr"_hash:
      switch (ioss_cell_points)
      {
        case 18:
        case 19:
          permutation = { /* corners */
            2, 3, 0, 1, 4,
            /* mid-edge points */
            7, 8, 5, 6, 11, 12, 9, 10,
            /* mid-face points */
            17, 15, 16, 13, 14
          };
          if (ioss_cell_points == 18)
          {
            break;
          }
          permutation.push_back(18);
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
#endif
  return !permutation.empty();
}

bool GetConnectivity(Ioss::GroupingEntity* group_entity, vtkCellGrid* grid, vtkDGCell* meta,
  int ioss_cell_points, vtkIOSSUtilities::Cache* cache)
{
  if (!group_entity || !meta)
  {
    return false;
  }
  auto& cellSpec = meta->GetCellSpec();
  if (cache)
  {
    cellSpec.Connectivity =
      vtkDataArray::SafeDownCast(cache->Find(group_entity, "__vtk_cell_connectivity__"));
    cellSpec.NodalGhostMarks =
      vtkDataArray::SafeDownCast(cache->Find(group_entity, "__vtk_point_ghosts__"));
  }

  if (!cellSpec.Connectivity)
  {
    std::vector<int> permutation;
    auto transform = std::unique_ptr<Ioss::Transform>(Ioss::TransformFactory::create("offset"));
    transform->set_property("offset", -1);
    auto ids_raw = vtkIOSSUtilities::GetData(group_entity, "connectivity_raw", transform.get());
    // Transfer ownership to a vtkDataSetAttributes instance:
    grid->GetAttributes(meta->GetClassName())->AddArray(ids_raw);
    if (ConnectivityNeedsPermutation(meta, ioss_cell_points, /* ioss_cell_order, */ permutation))
    {
      SwizzleComponents(ids_raw, permutation);
    }
    ids_raw->SetNumberOfComponents(ioss_cell_points);
    cellSpec.Connectivity = ids_raw;
    if (cache)
    {
      cache->Insert(group_entity, "__vtk_cell_connectivity__", cellSpec.Connectivity);
    }
  }

  if (!cellSpec.NodalGhostMarks)
  {
    // TODO: In ThirdParty/ioss/vtkioss/: use Ioss_CommSet.h or possibly
    //       exodus/Ioex_DecompositionData to obtain ghost-node flags and
    //       add to vtkDGCell::Source::NodalGhostMarks.
#if 0
    if (cache)
    {
      cache->Insert(group_entity, "__vtk_cell_connectivity__", cellSpec.NodalGhostMarks);
    }
#endif
  }
  return !!cellSpec.Connectivity;
}

vtkSmartPointer<vtkCellMetadata> GetCellMetadata(Ioss::GroupingEntity* group_entity,
  int& ioss_cell_points, int& ioss_cell_order, vtkCellGrid* cell_grid,
  vtkIOSSUtilities::Cache* cache)
{
  (void)cache;
  vtkSmartPointer<vtkCellMetadata> metadata;
  if (group_entity->get_property("entity_count").get_int() <= 0)
  {
    return metadata;
  }

  if (group_entity->property_exists("topology_type"))
  {
    auto topology_type = group_entity->get_property("topology_type").get_string();
    auto topology_element = Ioss::ElementTopology::factory(topology_type);
    metadata = vtkIOSSCellGridUtilities::GetCellMetadata(
      topology_element, ioss_cell_points, ioss_cell_order, cell_grid);
    cell_grid->AddCellMetadata(metadata);
  }
  return metadata;
}

bool GetShape(Ioss::Region* region, Ioss::GroupingEntity* group_entity,
  vtkCellAttribute::CellTypeInfo& cellShapeInfo, int timestep, vtkDGCell* meta, vtkCellGrid* grid,
  vtkIOSSUtilities::Cache* cache)
{
  (void)timestep;
  (void)group_entity; // TODO: If we ever squeeze points on a per-block basis, we must cache
                      // the nodal coords on group_entity (not nodeblock_entity) and
                      // use the vtkDGCell's cellSpec.Connectivity to subset points when
                      // generating the cache entry.

  auto nodeblock_entity = region->get_entity("nodeblock_1", Ioss::EntityType::NODEBLOCK);
  if (!nodeblock_entity)
  {
    return false;
  }
  vtkSmartPointer<vtkDataArray> cached = (cache
      ? vtkDataArray::SafeDownCast(cache->Find(nodeblock_entity, "__vtk_mesh_model_coordinates__"))
      : nullptr);
  if (cached)
  {
    vtkLogF(TRACE, "using cached mesh_model_coordinates");
  }
  else
  {
    cached = vtkIOSSUtilities::GetData(nodeblock_entity, "mesh_model_coordinates");
    cached = ChangeComponents(cached, 3);

    if (cache)
    {
      cache->Insert(group_entity, "__vtk_mesh_model_coordinates__", cached.GetPointer());
    }
  }

  grid->GetAttributes("coordinates")->AddArray(cached);

  vtkNew<vtkCellAttribute> attribute;
  attribute->Initialize("shape", "ℝ³", 3);
  cellShapeInfo.DOFSharing = "coordinates"_token; // Required for the shape attribute.
  cellShapeInfo.FunctionSpace = "HGRAD"_token;    // Required for the shape attribute.
  cellShapeInfo.ArraysByRole["connectivity"] = meta->GetCellSpec().Connectivity;
  cellShapeInfo.ArraysByRole["values"] = cached;
  attribute->SetCellTypeInfo(meta->GetClassName(), cellShapeInfo);
  grid->AddCellAttribute(attribute);
  grid->SetShapeAttribute(attribute);
  return true;
}

VTK_ABI_NAMESPACE_END
} // namespace vtkIOSSCellGridUtilities
