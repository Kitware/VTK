/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIOSSUtilities.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPIController.h"
#include "vtk_mpi.h"
#endif

#include "vtkIOSSUtilities.h"

#include "vtkArrayDispatch.h"
#include "vtkCellArray.h"
#include "vtkCellType.h"
#include "vtkDataSet.h"
#include "vtkGenericCell.h"
#include "vtkIdTypeArray.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"

#include <vtksys/RegularExpression.hxx>
#include <vtksys/SystemTools.hxx>

#include <Ioss_ElementTopology.h>
#include <Ioss_Field.h>
#include <Ioss_NodeBlock.h>
#include <Ioss_SideBlock.h>
#include <Ioss_SideSet.h>

#include <memory>

namespace vtkIOSSUtilities
{

//----------------------------------------------------------------------------
class Cache::CacheInternals
{
public:
  using KeyType = std::pair<std::string, std::string>;
  using ValueType = std::pair<vtkSmartPointer<vtkObject>, bool>;
  std::map<KeyType, ValueType> CacheMap;

  static std::string GetPath(const Ioss::GroupingEntity* entity)
  {
    std::ostringstream stream;
    auto e = entity;
    while (e)
    {
      stream << e->generic_name() << "#" << e->name();
      auto parent = e->contained_in();
      if (parent == e)
      {
        break;
      }
      if (parent)
      {
        stream << '/';
      }
      e = parent;
    }
    stream << ":"
           << vtksys::SystemTools::GetFilenameName(entity->get_database()->decoded_filename());
    return stream.str();
  }
};

//----------------------------------------------------------------------------
Cache::Cache()
  : Internals(new Cache::CacheInternals())
{
}

//----------------------------------------------------------------------------
Cache::~Cache()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
void Cache::ResetAccessCounts()
{
  for (auto& pair : this->Internals->CacheMap)
  {
    pair.second.second = false;
  }
}

//----------------------------------------------------------------------------
void Cache::ClearUnused()
{
  auto& internals = (*this->Internals);
  auto iter = internals.CacheMap.begin();
  while (iter != internals.CacheMap.end())
  {
    if (!iter->second.second)
    {
      iter = internals.CacheMap.erase(iter);
    }
    else
    {
      ++iter;
    }
  }
}

//----------------------------------------------------------------------------
void Cache::Clear()
{
  auto& internals = (*this->Internals);
  internals.CacheMap.clear();
}

//----------------------------------------------------------------------------
vtkObject* Cache::Find(const Ioss::GroupingEntity* entity, const std::string& cachekey) const
{
  auto& internals = (*this->Internals);
  auto key = CacheInternals::KeyType(CacheInternals::GetPath(entity), cachekey);
  auto iter = internals.CacheMap.find(key);
  if (iter != internals.CacheMap.end())
  {
    iter->second.second = true;
    return iter->second.first.GetPointer();
  }
  return nullptr;
}

//----------------------------------------------------------------------------
void Cache::Insert(
  const Ioss::GroupingEntity* entity, const std::string& cachekey, vtkObject* array)
{
  auto& internals = (*this->Internals);
  auto key = CacheInternals::KeyType(CacheInternals::GetPath(entity), cachekey);
  auto& value = internals.CacheMap[key];
  value.first = array;
  value.second = true;
}

//============================================================================
CaptureNonErrorMessages::CaptureNonErrorMessages()
  : DebugStream(&Ioss::Utils::get_debug_stream())
  , WarningStream(&Ioss::Utils::get_warning_stream())
{
  Ioss::Utils::set_debug_stream(this->Stream);
  Ioss::Utils::set_warning_stream(this->Stream);
}

//----------------------------------------------------------------------------
CaptureNonErrorMessages::~CaptureNonErrorMessages()
{
  Ioss::Utils::set_warning_stream(*this->WarningStream);
  Ioss::Utils::set_debug_stream(*this->DebugStream);
}

//----------------------------------------------------------------------------
std::string CaptureNonErrorMessages::GetMessages() const
{
  return this->Stream.str();
}

//============================================================================
//----------------------------------------------------------------------------
std::vector<std::pair<int, double>> GetTime(const Ioss::Region* region)
{
  const auto mxtime = region->get_max_time();
  if (mxtime.first <= 0)
  {
    // timestep index is 1-based, 0 implies time is not present in the dataset.
    return {};
  }

  const auto mntime = region->get_min_time();

  std::vector<std::pair<int, double>> result;
  for (int cc = mntime.first; cc <= mxtime.first; ++cc)
  {
    result.emplace_back(cc, region->get_state_time(cc));
  }
  return result;
}

//----------------------------------------------------------------------------
Ioss::EntityType GetIOSSEntityType(vtkIOSSReader::EntityType vtk_type)
{
  switch (vtk_type)
  {
    case vtkIOSSReader::NODEBLOCK:
      return Ioss::EntityType::NODEBLOCK;
    case vtkIOSSReader::EDGEBLOCK:
      return Ioss::EntityType::EDGEBLOCK;
    case vtkIOSSReader::FACEBLOCK:
      return Ioss::EntityType::FACEBLOCK;
    case vtkIOSSReader::ELEMENTBLOCK:
      return Ioss::EntityType::ELEMENTBLOCK;
    case vtkIOSSReader::STRUCTUREDBLOCK:
      return Ioss::EntityType::STRUCTUREDBLOCK;
    case vtkIOSSReader::NODESET:
      return Ioss::EntityType::NODESET;
    case vtkIOSSReader::EDGESET:
      return Ioss::EntityType::EDGESET;
    case vtkIOSSReader::FACESET:
      return Ioss::EntityType::FACESET;
    case vtkIOSSReader::ELEMENTSET:
      return Ioss::EntityType::ELEMENTSET;
    case vtkIOSSReader::SIDESET:
      return Ioss::EntityType::SIDESET;
    default:
      throw std::runtime_error("Invalid entity type " + std::to_string(vtk_type));
  }
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkDataArray> CreateArray(const Ioss::Field& field)
{
  // NOTE: if adding new array types here, ensure that
  // vtkIOSSUtilities::ArrayList is updated.
  vtkSmartPointer<vtkDataArray> array;
  switch (field.get_type())
  {
    case Ioss::Field::DOUBLE:
      array.TakeReference(vtkDoubleArray::New());
      break;
    case Ioss::Field::INT32:
      array.TakeReference(vtkTypeInt32Array::New());
      break;
    case Ioss::Field::INT64:
      array.TakeReference(vtkTypeInt64Array::New());
      break;
    default:
      throw std::runtime_error("Unsupported field type " + std::to_string(field.get_type()));
  }
  array->SetName(field.get_name().c_str());
  array->SetNumberOfComponents(field.raw_storage()->component_count());
  array->SetNumberOfTuples(static_cast<vtkIdType>(field.raw_count()));
  vtkLogIfF(ERROR,
    static_cast<vtkIdType>(field.get_size()) != (array->GetDataSize() * array->GetDataTypeSize()),
    "Size mismatch ioss-size=%d, vtk-size: %d", (int)field.get_size(),
    (int)(array->GetDataSize() * array->GetDataTypeSize()));
  if (static_cast<vtkIdType>(field.get_size()) != array->GetDataSize() * array->GetDataTypeSize())
  {
    throw std::runtime_error("Incorrect array size");
  }
  return array;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkDataArray> GetData(const Ioss::GroupingEntity* entity,
  const std::string& fieldname, Ioss::Transform* transform /* = nullptr*/,
  Cache* cache /*=nullptr*/, const std::string& cachekey /*=std::string()*/)
{
  const auto key = cachekey.empty() ? fieldname : cachekey;
  if (cache)
  {
    if (auto cached = vtkDataArray::SafeDownCast(cache->Find(entity, key)))
    {
      vtkLogF(TRACE, "using cached %s", fieldname.c_str());
      return cached;
    }
  }

  auto field = entity->get_field(fieldname); // <-- throws std::runtime_error

  // vtkLogF(TRACE, "%s: size: %d * %d", fieldname.c_str(), (int)field.raw_count(),
  //  (int)field.raw_storage()->component_count());
  auto array = vtkIOSSUtilities::CreateArray(field);
  auto count = entity->get_field_data(
    fieldname, array->GetVoidPointer(0), array->GetDataSize() * array->GetDataTypeSize());
  if (static_cast<vtkIdType>(count) != array->GetNumberOfTuples())
  {
    throw std::runtime_error("Failed to read field " + fieldname);
  }
  if (transform)
  {
    field.add_transform(transform);
    field.transform(array->GetVoidPointer(0));
  }

  if (cache)
  {
    cache->Insert(entity, key, array);
  }
  return array;
}

//----------------------------------------------------------------------------
int GetCellType(const Ioss::ElementTopology* topology)
{
  switch (topology->shape())
  {
    case Ioss::ElementShape::SPHERE:
      return VTK_VERTEX;

    case Ioss::ElementShape::POINT:
      return VTK_POLY_VERTEX;

    case Ioss::ElementShape::SPRING:
    case Ioss::ElementShape::LINE:
      switch (topology->number_nodes())
      {
        case 2:
          return VTK_LINE;
        case 3:
          return VTK_QUADRATIC_EDGE;
      }
      break;

    case Ioss::ElementShape::TRI:
      switch (topology->number_nodes())
      {
        case 6:
          return VTK_QUADRATIC_TRIANGLE;
        case 4:
        case 3:
          return VTK_TRIANGLE;
      }
      break;
    case Ioss::ElementShape::QUAD:
      switch (topology->number_nodes())
      {
        case 8:
          return VTK_QUADRATIC_QUAD;
        case 9:
          return VTK_BIQUADRATIC_QUAD;
        case 4:
          return VTK_QUAD;
      }
      break;
    case Ioss::ElementShape::TET:
      switch (topology->number_nodes())
      {
        case 10:
        case 11:
          return VTK_QUADRATIC_TETRA;
        case 15:
          return VTK_LAGRANGE_TETRAHEDRON;
        case 8:
        case 4:
          return VTK_TETRA;
      }
      break;
    case Ioss::ElementShape::PYRAMID:
      switch (topology->number_nodes())
      {
        case 13:
        case 14:
          return VTK_QUADRATIC_PYRAMID;
        case 19:
          return VTK_TRIQUADRATIC_PYRAMID;
        case 5:
          return VTK_PYRAMID;
      }
      break;
    case Ioss::ElementShape::WEDGE:
      switch (topology->number_nodes())
      {
        case 6:
          return VTK_WEDGE;
        case 12:
          return VTK_QUADRATIC_LINEAR_WEDGE;
        case 15:
          return VTK_QUADRATIC_WEDGE;
        case 18:
          return VTK_BIQUADRATIC_QUADRATIC_WEDGE;
        case 21:
          return VTK_LAGRANGE_WEDGE;
      }
      break;
    case Ioss::ElementShape::HEX:
      switch (topology->number_nodes())
      {
        case 8:
          return VTK_HEXAHEDRON;
        case 20:
          return VTK_QUADRATIC_HEXAHEDRON;
        case 27:
          return VTK_TRIQUADRATIC_HEXAHEDRON;
      }
      break;

    case Ioss::ElementShape::UNKNOWN:
      // this happens for superelements, we just return points
      // for such elements (see paraview/paraview#19154).
      return VTK_POLY_VERTEX;

    default:
      break;
  }
  vtkLogF(ERROR, "Element of topology '%s' with %d nodes is not supported.",
    topology->name().c_str(), topology->number_nodes());
  throw std::runtime_error("Unsupported topology " + topology->name());
}

//----------------------------------------------------------------------------
const Ioss::ElementTopology* GetElementTopology(int vtk_cell_type)
{
  const char* elementType = nullptr;
  switch (vtk_cell_type)
  {
    case VTK_VERTEX:
    case VTK_POLY_VERTEX:
      elementType = "point";
      break;
    case VTK_LINE:
      elementType = "edge2";
      break;
    case VTK_QUADRATIC_EDGE:
      elementType = "edge4";
      break;
    case VTK_TRIANGLE:
      elementType = "tri3";
      break;
    case VTK_QUADRATIC_TRIANGLE:
      elementType = "tri6";
      break;
    case VTK_QUAD:
      elementType = "quad4";
      break;
    case VTK_QUADRATIC_QUAD:
      elementType = "quad8";
      break;
    case VTK_BIQUADRATIC_QUAD:
      elementType = "quad9";
      break;
    case VTK_TETRA:
      elementType = "tet4";
      break;
    case VTK_QUADRATIC_TETRA:
      elementType = "tet11";
      break;
    case VTK_LAGRANGE_TETRAHEDRON:
      elementType = "tet15";
      break;
    case VTK_QUADRATIC_PYRAMID:
      elementType = "pyramid13";
      break;
    case VTK_TRIQUADRATIC_PYRAMID:
      elementType = "pyramid19";
      break;
    case VTK_PYRAMID:
      elementType = "pyramid5";
      break;
    case VTK_QUADRATIC_WEDGE:
      elementType = "wedge15";
      break;
    case VTK_BIQUADRATIC_QUADRATIC_WEDGE:
      elementType = "wedge18";
      break;
    case VTK_LAGRANGE_WEDGE:
      elementType = "wedge21";
      break;
    case VTK_WEDGE:
      elementType = "wedge6";
      break;
    case VTK_HEXAHEDRON:
      elementType = "hex8";
      break;
    case VTK_QUADRATIC_HEXAHEDRON:
      elementType = "hex20";
      break;
    case VTK_TRIQUADRATIC_HEXAHEDRON:
      elementType = "hex27";
      break;
  }

  if (auto* element = elementType ? Ioss::ElementTopology::factory(elementType) : nullptr)
  {
    return element;
  }

  vtkLogF(ERROR, "VTK cell type (%d) cannot be mapped to an Ioss element type!", vtk_cell_type);
  throw std::runtime_error("Unsupported cell type " + std::to_string(vtk_cell_type));
}

//----------------------------------------------------------------------------
// internal: get number of points in VTK cell type.
static vtkIdType GetNumberOfPointsInCellType(int vtk_cell_type)
{
  switch (vtk_cell_type)
  {
    case VTK_POLY_VERTEX:
      return -1;
    default:
      break;
  }

  vtkNew<vtkGenericCell> cell;
  cell->SetCellType(vtk_cell_type);
  return cell->GetNumberOfPoints();
}

//----------------------------------------------------------------------------
// internal: change components helper.
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

//----------------------------------------------------------------------------
// internal: change components.
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

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
vtkSmartPointer<vtkCellArray> GetConnectivity(
  Ioss::GroupingEntity* group_entity, int& vtk_topology_type, Cache* cache /*=nullptr*/)
{
  if (group_entity->get_property("entity_count").get_int() <= 0)
  {
    vtk_topology_type = VTK_EMPTY_CELL;
    return nullptr;
  }

  vtkIdType ioss_cell_points = -1;
  if (group_entity->property_exists("topology_type"))
  {
    auto topology_type = group_entity->get_property("topology_type").get_string();
    auto topology_element = Ioss::ElementTopology::factory(topology_type);
    vtk_topology_type = vtkIOSSUtilities::GetCellType(topology_element);
    ioss_cell_points = static_cast<vtkIdType>(topology_element->number_nodes());

    vtkLogF(TRACE, "topology_type=%s, number_nodes=%d", topology_type.c_str(),
      topology_element->number_nodes());
  }
  else if (group_entity->type() == Ioss::EntityType::NODESET)
  {
    // this happens for NODESETs.
    vtk_topology_type = VTK_VERTEX;
  }
  else
  {
    throw std::runtime_error(
      "Unexpected group_entity for `GetConnectivity` call: " + group_entity->name());
  }

  if (auto cached =
        (cache ? vtkCellArray::SafeDownCast(cache->Find(group_entity, "__vtk_cell__array__"))
               : nullptr))
  {
    vtkLogF(TRACE, "using cached connectivity");
    return cached;
  }

  if (group_entity->type() == Ioss::EntityType::NODESET)
  {
    // for nodesets, we create a cell array with single cells.

    // ioss ids_raw is 1-indexed, let's make it 0-indexed for VTK.
    auto transform = std::unique_ptr<Ioss::Transform>(Iotr::Factory::create("offset"));
    transform->set_property("offset", -1);
    auto ids_raw = vtkIOSSUtilities::GetData(group_entity, "ids_raw", transform.get());
    ids_raw->SetNumberOfComponents(1);

    vtkSmartPointer<vtkCellArray> cellArray = vtkSmartPointer<vtkCellArray>::New();
    if (!cellArray->SetData(1, ids_raw))
    {
      throw std::runtime_error("Error converting connectivity to vtkCellArray!");
    }
    if (cache)
    {
      cache->Insert(group_entity, "__vtk_cell__array__", cellArray);
    }
    return cellArray;
  }

  vtkSmartPointer<vtkCellArray> cellArray = vtkSmartPointer<vtkCellArray>::New();

  // ioss connectivity_raw is 1-indexed, let's make it 0-indexed for VTK.
  auto transform = std::unique_ptr<Ioss::Transform>(Iotr::Factory::create("offset"));
  transform->set_property("offset", -1);

  auto connectivity_raw =
    vtkIOSSUtilities::GetData(group_entity, "connectivity_raw", transform.get());

  auto vtk_cell_points = vtkIOSSUtilities::GetNumberOfPointsInCellType(vtk_topology_type);
  if (vtk_cell_points == -1)
  {
    // means that the VTK cell can have as many points as needed e.g.
    // VTK_POLY_VERTEX.
    vtk_cell_points = ioss_cell_points;
  }
  else if (vtk_cell_points < ioss_cell_points)
  {
    // need to drop components in the 'connectivity_raw' array since we don't
    // support all components in VTK cell.
    vtkLogF(TRACE, "IOSS has more points for this cell than VTK. Skipping the extra components.");
    connectivity_raw = vtkIOSSUtilities::ChangeComponents(connectivity_raw, vtk_cell_points);
  }
  else if (vtk_cell_points > ioss_cell_points)
  {
    throw std::runtime_error("VTK cell requires more points than provided!");
  }

  // IOSS cells and VTK cells need not have same point ordering. If that's the
  // case, we need to transform them.
  // Here, using the indexes specified in Ioss docs (which are 1-based), just
  // add them so that the cell is ordered correctly in VTK.
  // ref: https://gsjaardema.github.io/seacas-docs/html/element_types.html
  std::vector<int> ordering_transform;
  switch (vtk_topology_type)
  {
    case VTK_WEDGE:
      ordering_transform = std::vector<int>{ 4, 5, 6, 1, 2, 3 };
      break;

    case VTK_QUADRATIC_WEDGE: // wedge-15
      // clang-format off
      ordering_transform = std::vector<int>{
        4, 5, 6, 1, 2, 3,
        13, 14, 15,
        7, 8, 9,
        10, 11, 12
      };
      // clang-format on
      break;

    case VTK_BIQUADRATIC_QUADRATIC_WEDGE: // wedge-18
      // clang-format off
      ordering_transform = std::vector<int>{
        /* 2 triangles */
        4, 5, 6, 1, 2, 3,

        /* edge centers */
        13, 14, 15,
        7, 8, 9,
        10, 11, 12,

        /* quad-centers */
        16, 17, 18
      };
      // clang-format on
      break;

    case VTK_LAGRANGE_WEDGE: // wedge-21
      // here, the ordering is consistent with IOSS!
      // so don't do anything.
      break;

    case VTK_QUADRATIC_HEXAHEDRON: // hex-20
      // clang-format off
      ordering_transform = std::vector<int>{
        /* 8 corners */
        1, 2, 3, 4,
        5, 6, 7, 8,

        /* 12 mid-edge nodes */
        9, 10, 11, 12,
        17, 18, 19, 20,
        13, 14, 15, 16
      };
      // clang-format on
      break;

    case VTK_TRIQUADRATIC_HEXAHEDRON: // hex-27
      // clang-format off
      ordering_transform = std::vector<int>{
        /* 8 corners */
        1, 2, 3, 4,
        5, 6, 7, 8,

        /* 12 mid-edge nodes */
        9, 10, 11, 12,
        17, 18, 19, 20,
        13, 14, 15, 16,

        /* 6 mid-face nodes */
        24, 25, 26, 27, 22, 23,

        /* mid-volume node*/
        21
      };
      // clang-format on
      break;
    default:
      break;
  }

  if (!ordering_transform.empty())
  {
    // offset by 1 to make 0-based.
    for (auto& val : ordering_transform)
    {
      val -= 1;
    }
    assert(static_cast<decltype(vtk_cell_points)>(ordering_transform.size()) == vtk_cell_points);
    vtkIOSSUtilities::SwizzleComponents(connectivity_raw, ordering_transform);
  }

  // change number of components to 1.
  connectivity_raw->SetNumberOfComponents(1);
  if (!cellArray->SetData(vtk_cell_points, connectivity_raw))
  {
    throw std::runtime_error("Error converting connectivity to vtkCellArray!");
  }

  if (cache)
  {
    cache->Insert(group_entity, "__vtk_cell__array__", cellArray);
  }
  return cellArray;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkPoints> GetMeshModelCoordinates(
  const Ioss::GroupingEntity* group_entity, Cache* cache /*=nullptr*/)
{
  if (auto cached = (cache
          ? vtkPoints::SafeDownCast(cache->Find(group_entity, "__vtk_mesh_model_coordinates__"))
          : nullptr))
  {
    vtkLogF(TRACE, "using cached mesh_model_coordinates");
    return cached;
  }

  auto mesh_model_coordinates = vtkIOSSUtilities::GetData(group_entity, "mesh_model_coordinates");
  mesh_model_coordinates = vtkIOSSUtilities::ChangeComponents(mesh_model_coordinates, 3);
  vtkNew<vtkPoints> pts;
  pts->SetData(mesh_model_coordinates);

  if (cache)
  {
    cache->Insert(group_entity, "__vtk_mesh_model_coordinates__", pts.GetPointer());
  }
  return pts;
}

//----------------------------------------------------------------------------
bool IsFieldTransient(Ioss::GroupingEntity* entity, const std::string& fieldname)
{
  if (entity->type() == Ioss::EntityType::SIDESET)
  {
    auto sideSet = static_cast<Ioss::SideSet*>(entity);
    bool is_transient = !sideSet->get_side_blocks().empty();
    for (auto& sideBlock : sideSet->get_side_blocks())
    {
      is_transient &= IsFieldTransient(sideBlock, fieldname);
    }
    return is_transient;
  }
  else
  {
    return (entity->field_exists(fieldname) &&
      (entity->get_fieldref(fieldname).get_role() == Ioss::Field::RoleType::TRANSIENT ||
        entity->get_fieldref(fieldname).get_role() == Ioss::Field::RoleType::REDUCTION));
  }
}

//----------------------------------------------------------------------------
std::string GetDisplacementFieldName(Ioss::GroupingEntity* nodeblock)
{
  if (nodeblock == nullptr)
  {
    return std::string();
  }

  assert(nodeblock->type() == Ioss::EntityType::NODEBLOCK);

  Ioss::NameList names;
  nodeblock->field_describe(Ioss::Field::TRANSIENT, &names);

  const int degree = nodeblock->get_property("component_degree").get_int();
  // find the first field that begins with "dis" and has as many components as
  // the degree.
  for (const auto& fname : names)
  {
    if (vtksys::SystemTools::UpperCase(fname.substr(0, 3)) == "DIS" &&
      nodeblock->get_fieldref(fname).raw_storage()->component_count() == degree)
    {
      return fname;
    }
  }

  return std::string();
}

//----------------------------------------------------------------------------
std::string GetDisplacementFieldName(vtkDataSet* dataset)
{
  if (dataset == nullptr)
  {
    return std::string();
  }

  auto* pd = dataset->GetPointData();
  for (int cc = 0, max = pd->GetNumberOfArrays(); cc < max; ++cc)
  {
    auto* array = pd->GetArray(cc);
    std::string arrayName = (array && array->GetName()) ? array->GetName() : "";
    if (vtksys::SystemTools::UpperCase(arrayName.substr(0, 3)) == "DIS" &&
      array->GetNumberOfComponents() == 3)
    {
      // while not true currently, once paraview/paraview#21237 is fixed, all
      // displacement vectors will be 3 component arrays.
      return arrayName;
    }
  }

  return std::string();
}

//----------------------------------------------------------------------------
DatabaseFormatType DetectType(const std::string& dbaseName)
{
  // clang-format off
  auto name = vtksys::SystemTools::LowerCase(dbaseName);
  if (name == "catalyst.bin")
  {
    return DatabaseFormatType::CATALYST;
  }

  vtksys::RegularExpression extensionRegexCGNS(R"(^.*\.(cgns[^-.]*))");
  // clang-format on
  if (extensionRegexCGNS.find(name) && extensionRegexCGNS.match(1) == "cgns")
  {
    return DatabaseFormatType::CGNS;
  }

  return DatabaseFormatType::EXODUS;
}

//----------------------------------------------------------------------------
DatabaseFormatType GetFormat(const Ioss::GroupingEntity* entity)
{
  assert(entity != nullptr && entity->get_database() != nullptr);
  if (entity->get_database()->get_format() == "CGNS")
  {
    return DatabaseFormatType::CGNS;
  }
  else if (entity->get_database()->get_format() == "CATALYST2")
  {
    return DatabaseFormatType::CATALYST;
  }
  else
  {
    return DatabaseFormatType::EXODUS;
  }
}

//----------------------------------------------------------------------------
// Implementation detail for Schwarz counter idiom.
class vtkIOSSUtilitiesCleanup
{
public:
  vtkIOSSUtilitiesCleanup();
  ~vtkIOSSUtilitiesCleanup();

private:
  vtkIOSSUtilitiesCleanup(const vtkIOSSUtilitiesCleanup&) = delete;
  void operator=(const vtkIOSSUtilitiesCleanup&) = delete;
};
static vtkIOSSUtilitiesCleanup vtkIOSSUtilitiesCleanupInstance;

static unsigned int vtkIOSSUtilitiesCleanupCounter = 0;
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
static vtkMPIController* vtkIOSSUtilitiesCleanupMPIController = nullptr;
#endif

vtkIOSSUtilitiesCleanup::vtkIOSSUtilitiesCleanup()
{
  ++vtkIOSSUtilitiesCleanupCounter;
}

vtkIOSSUtilitiesCleanup::~vtkIOSSUtilitiesCleanup()
{
  if (--vtkIOSSUtilitiesCleanupCounter == 0)
  {
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
    if (vtkIOSSUtilitiesCleanupMPIController)
    {
      vtkLogF(TRACE, "Cleaning up MPI controller created for Ioss filters.");
      vtkIOSSUtilitiesCleanupMPIController->Finalize();
      vtkIOSSUtilitiesCleanupMPIController->Delete();
      vtkIOSSUtilitiesCleanupMPIController = nullptr;
    }
#endif
  }
}

//----------------------------------------------------------------------------
void InitializeEnvironmentForIOSS()
{
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  int mpiOk;
  MPI_Initialized(&mpiOk);
  if (!mpiOk)
  {
    vtkLogF(TRACE,
      "Initializing MPI for Ioss filters since process did not do so in an MPI enabled build.");
    assert(vtkIOSSUtilitiesCleanupMPIController == nullptr);
    vtkIOSSUtilitiesCleanupMPIController = vtkMPIController::New();

    static int argc = 0;
    static char** argv = { nullptr };
    vtkIOSSUtilitiesCleanupMPIController->Initialize(&argc, &argv);
  }
#endif
}

//----------------------------------------------------------------------------
std::string GetSanitizedBlockName(const Ioss::Region* region, const std::string& blockname)
{
  if (vtkIOSSUtilities::GetFormat(region) != vtkIOSSUtilities::DatabaseFormatType::CGNS)
  {
    return blockname;
  }

  // clang-format off
  vtksys::RegularExpression regex(R"(_proc-[0-9]+)");
  // clang-format on
  std::string newname = blockname;
  while (regex.find(newname))
  {
    newname.erase(
      std::next(newname.begin(), regex.start()), std::next(newname.begin(), regex.end()));
  }

  return newname;
}

//----------------------------------------------------------------------------
std::vector<Ioss::StructuredBlock*> GetMatchingStructuredBlocks(
  Ioss::Region* region, const std::string& blockname)
{
  std::vector<Ioss::StructuredBlock*> groups;
  for (auto block : region->get_structured_blocks())
  {
    if (block->name() == blockname ||
      vtkIOSSUtilities::GetSanitizedBlockName(region, block->name()) == blockname)
    {
      groups.push_back(block);
    }
  }

  return groups;
}

//----------------------------------------------------------------------------
template <>
void GetEntityAndFieldNames<Ioss::SideSet>(const Ioss::Region* region,
  const std::vector<Ioss::SideSet*>& entities, std::set<EntityNameType>& entity_names,
  std::set<std::string>& field_names)
{
  for (const auto& entity : entities)
  {
    const int64_t id = entity->property_exists("id") ? entity->get_property("id").get_int() : 0;
    auto name = vtkIOSSUtilities::GetSanitizedBlockName(region, entity->name());
    entity_names.insert(EntityNameType{ static_cast<vtkTypeUInt64>(id), name });

    for (const auto& block : entity->get_side_blocks())
    {
      Ioss::NameList attributeNames;
      block->field_describe(Ioss::Field::TRANSIENT, &attributeNames);
      block->field_describe(Ioss::Field::ATTRIBUTE, &attributeNames);
      std::copy(attributeNames.begin(), attributeNames.end(),
        std::inserter(field_names, field_names.end()));
    }

    // not sure if there will ever be any fields on the side-set itself, but no
    // harm in checking.
    Ioss::NameList attributeNames;
    entity->field_describe(Ioss::Field::TRANSIENT, &attributeNames);
    entity->field_describe(Ioss::Field::ATTRIBUTE, &attributeNames);
    std::copy(
      attributeNames.begin(), attributeNames.end(), std::inserter(field_names, field_names.end()));
  }
}

} // end of namespace.
