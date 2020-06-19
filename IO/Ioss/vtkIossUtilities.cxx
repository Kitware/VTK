/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIossUtilities.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkIossUtilities.h"

#include "vtkArrayDispatch.h"
#include "vtkCellArray.h"
#include "vtkCellType.h"
#include "vtkGenericCell.h"
#include "vtkIdTypeArray.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"

#include <vtksys/SystemTools.hxx>

#include <Ioss_ElementTopology.h>
#include <Ioss_Field.h>
#include <Ioss_NodeBlock.h>
#include <Ioss_SideBlock.h>
#include <Ioss_SideSet.h>

#include <memory>

namespace vtkIossUtilities
{

//----------------------------------------------------------------------------
class Cache::CacheInternals
{
public:
  using KeyType = std::pair<const void*, std::string>;
  using ValueType = std::pair<vtkSmartPointer<vtkObject>, bool>;
  std::map<KeyType, ValueType> CacheMap;
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
    if (iter->second.second == false)
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
  auto key = CacheInternals::KeyType(entity, cachekey);
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
  auto key = CacheInternals::KeyType(entity, cachekey);
  auto& value = internals.CacheMap[key];
  value.first = array;
  value.second = true;
}

//----------------------------------------------------------------------------
std::set<double> GetTimeValues(const Ioss::Region* region)
{
  const auto mxtime = region->get_max_time();
  if (mxtime.first <= 0)
  {
    // timestep index is 1-based, 0 implies time is not present in the dataset.
    return std::set<double>{};
  }

  const auto mntime = region->get_min_time();
  std::set<double> timevalues;
  for (int cc = mntime.first; cc <= mxtime.first; ++cc)
  {
    timevalues.insert(region->get_state_time(cc));
  }
  return timevalues;
}

//----------------------------------------------------------------------------
Ioss::EntityType GetIossEntityType(vtkIossReader::EntityType vtk_type)
{
  switch (vtk_type)
  {
    case vtkIossReader::NODEBLOCK:
      return Ioss::EntityType::NODEBLOCK;
    case vtkIossReader::EDGEBLOCK:
      return Ioss::EntityType::EDGEBLOCK;
    case vtkIossReader::FACEBLOCK:
      return Ioss::EntityType::FACEBLOCK;
    case vtkIossReader::ELEMENTBLOCK:
      return Ioss::EntityType::ELEMENTBLOCK;
    case vtkIossReader::NODESET:
      return Ioss::EntityType::NODESET;
    case vtkIossReader::EDGESET:
      return Ioss::EntityType::EDGESET;
    case vtkIossReader::FACESET:
      return Ioss::EntityType::FACESET;
    case vtkIossReader::ELEMENTSET:
      return Ioss::EntityType::ELEMENTSET;
    case vtkIossReader::SIDESET:
      return Ioss::EntityType::SIDESET;
    default:
      throw std::runtime_error("Invalid entity type " + std::to_string(vtk_type));
  }
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkDataArray> CreateArray(const Ioss::Field& field)
{
  // NOTE: if adding new array types here, ensure that
  // vtkIossUtilities::ArrayList is updated.
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
  auto array = vtkIossUtilities::CreateArray(field);
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
    case Ioss::ElementShape::POINT:
      return VTK_POLY_VERTEX;

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
        case 5:
          return VTK_PYRAMID;
      }
      break;
    case Ioss::ElementShape::WEDGE:
      switch (topology->number_nodes())
      {
        case 15:
          return VTK_QUADRATIC_WEDGE;
        case 21:
          return VTK_LAGRANGE_WEDGE;
        case 18:
        case 6:
          return VTK_WEDGE;
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
  using SupportedArrays = vtkIossUtilities::ArrayList;
  using Dispatch = vtkArrayDispatch::DispatchByArray<SupportedArrays>;
  if (!Dispatch::Execute(result, worker))
  {
    std::runtime_error("Failed to strip extra components from array!");
  }
  return result;
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
    vtk_topology_type = vtkIossUtilities::GetCellType(topology_element);
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
    auto ids_raw = vtkIossUtilities::GetData(group_entity, "ids_raw", transform.get());
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
    vtkIossUtilities::GetData(group_entity, "connectivity_raw", transform.get());

  auto vtk_cell_points = vtkIossUtilities::GetNumberOfPointsInCellType(vtk_topology_type);
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
    connectivity_raw = vtkIossUtilities::ChangeComponents(connectivity_raw, vtk_cell_points);
  }
  else if (vtk_cell_points > ioss_cell_points)
  {
    throw std::runtime_error("VTK cell requires more points than provided!");
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
  Ioss::GroupingEntity* group_entity, Cache* cache /*=nullptr*/)
{
  if (auto cached = (cache
          ? vtkPoints::SafeDownCast(cache->Find(group_entity, "__vtk_mesh_model_coordinates__"))
          : nullptr))
  {
    vtkLogF(TRACE, "using cached mesh_model_coordinates");
    return cached;
  }

  auto mesh_model_coordinates = vtkIossUtilities::GetData(group_entity, "mesh_model_coordinates");
  mesh_model_coordinates = vtkIossUtilities::ChangeComponents(mesh_model_coordinates, 3);
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
    bool is_transient = (sideSet->get_side_blocks().size() > 0);
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

} // end of namespace.
