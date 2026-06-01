//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include <fides/FidesWriter.h>

#if FIDES_USE_MPI
#include <fides/fides_mpi.h>
#endif

#include <fides/internal/MpiHelper.h>
#include <fides/internal/predefined/DataModelHelperFunctions.h>

#include <adios2.h>

#include <fides_rapidjson.h>
// clang-format off
#include FIDES_RAPIDJSON(rapidjson/document.h)
#include FIDES_RAPIDJSON(rapidjson/prettywriter.h)
#include FIDES_RAPIDJSON(rapidjson/stringbuffer.h)
// clang-format on

#include <array>
#include <climits>
#include <cstdint>
#include <numeric>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#if SIZE_MAX == UCHAR_MAX
#define FIDES_MPI_SIZE_T MPI_UNSIGNED_CHAR
#elif SIZE_MAX == USHRT_MAX
#define FIDES_MPI_SIZE_T MPI_UNSIGNED_SHORT
#elif SIZE_MAX == UINT_MAX
#define FIDES_MPI_SIZE_T MPI_UNSIGNED
#elif SIZE_MAX == ULONG_MAX
#define FIDES_MPI_SIZE_T MPI_UNSIGNED_LONG
#elif SIZE_MAX == ULLONG_MAX
#define FIDES_MPI_SIZE_T MPI_UNSIGNED_LONG_LONG
#else
#define FIDES_MPI_SIZE_T MPI_UNSIGNED_LONG
#endif

namespace fides
{

namespace
{

/// Helper: write a typed buffer to ADIOS with selection.
template <typename T>
void PutTypedArray(adios2::IO& io,
                   adios2::Engine& engine,
                   const std::string& name,
                   const void* data,
                   const std::vector<size_t>& shape,
                   const std::vector<size_t>& offset,
                   const std::vector<size_t>& count)
{
  auto var = io.InquireVariable<T>(name);
  if (!var)
  {
    return;
  }
  var.SetShape(shape);
  adios2::Box<adios2::Dims> sel(offset, count);
  var.SetSelection(sel);
  engine.Put<T>(var, static_cast<const T*>(data));
}

/// Dispatch PutTypedArray based on DataType.
void PutRawArray(adios2::IO& io,
                 adios2::Engine& engine,
                 const std::string& name,
                 const RawArray& raw,
                 const std::vector<size_t>& shape,
                 const std::vector<size_t>& offset,
                 const std::vector<size_t>& count)
{
  switch (raw.Type)
  {
    case DataType::Float32:
      PutTypedArray<float>(io, engine, name, raw.Data.get(), shape, offset, count);
      break;
    case DataType::Float64:
      PutTypedArray<double>(io, engine, name, raw.Data.get(), shape, offset, count);
      break;
    case DataType::Int8:
      PutTypedArray<int8_t>(io, engine, name, raw.Data.get(), shape, offset, count);
      break;
    case DataType::Int16:
      PutTypedArray<int16_t>(io, engine, name, raw.Data.get(), shape, offset, count);
      break;
    case DataType::Int32:
      PutTypedArray<int32_t>(io, engine, name, raw.Data.get(), shape, offset, count);
      break;
    case DataType::Int64:
      PutTypedArray<int64_t>(io, engine, name, raw.Data.get(), shape, offset, count);
      break;
    case DataType::UInt8:
      PutTypedArray<uint8_t>(io, engine, name, raw.Data.get(), shape, offset, count);
      break;
    case DataType::UInt16:
      PutTypedArray<uint16_t>(io, engine, name, raw.Data.get(), shape, offset, count);
      break;
    case DataType::UInt32:
      PutTypedArray<uint32_t>(io, engine, name, raw.Data.get(), shape, offset, count);
      break;
    case DataType::UInt64:
      PutTypedArray<uint64_t>(io, engine, name, raw.Data.get(), shape, offset, count);
      break;
    default:
      throw std::runtime_error("PutRawArray: unsupported DataType");
  }
}

/// Define an ADIOS variable based on DataType.
template <typename T>
void DefineTypedVariable(adios2::IO& io,
                         const std::string& name,
                         const std::vector<size_t>& shape,
                         const std::vector<size_t>& offset,
                         const std::vector<size_t>& count)
{
  io.DefineVariable<T>(name, shape, offset, count);
}

void DefineVariable(adios2::IO& io,
                    const std::string& name,
                    DataType type,
                    const std::vector<size_t>& shape,
                    const std::vector<size_t>& offset,
                    const std::vector<size_t>& count)
{
  switch (type)
  {
    case DataType::Float32:
      DefineTypedVariable<float>(io, name, shape, offset, count);
      break;
    case DataType::Float64:
      DefineTypedVariable<double>(io, name, shape, offset, count);
      break;
    case DataType::Int8:
      DefineTypedVariable<int8_t>(io, name, shape, offset, count);
      break;
    case DataType::Int16:
      DefineTypedVariable<int16_t>(io, name, shape, offset, count);
      break;
    case DataType::Int32:
      DefineTypedVariable<int32_t>(io, name, shape, offset, count);
      break;
    case DataType::Int64:
      DefineTypedVariable<int64_t>(io, name, shape, offset, count);
      break;
    case DataType::UInt8:
      DefineTypedVariable<uint8_t>(io, name, shape, offset, count);
      break;
    case DataType::UInt16:
      DefineTypedVariable<uint16_t>(io, name, shape, offset, count);
      break;
    case DataType::UInt32:
      DefineTypedVariable<uint32_t>(io, name, shape, offset, count);
      break;
    case DataType::UInt64:
      DefineTypedVariable<uint64_t>(io, name, shape, offset, count);
      break;
    default:
      throw std::runtime_error("DefineVariable: unsupported DataType");
  }
}

/// Put an already-defined ADIOS2 local variable: resize its block to
/// \c numTuples (× \c numComp when > 1) and Put the buffer. Local variables
/// write one block per engine.Put call, which is what the cellgrid reader
/// iterates via BlocksInfo (one block per partition).
///
/// Multi-component data is stored as a 2-D block [numTuples, numComp] so
/// the per-tuple stride is preserved on read. The reader counts tuples
/// (e.g. cells via the connectivity variable's outer dimension), so a
/// flat 1-D block would mis-report the cell count.
template <typename T>
void PutLocalTyped(adios2::IO& io,
                   adios2::Engine& engine,
                   const std::string& name,
                   const void* data,
                   size_t numTuples,
                   size_t numComp = 1)
{
  auto var = io.InquireVariable<T>(name);
  if (!var)
  {
    return;
  }
  if (numComp > 1)
  {
    var.SetSelection({ {}, { numTuples, numComp } });
  }
  else
  {
    var.SetSelection({ {}, { numTuples } });
  }
  engine.Put<T>(var, static_cast<const T*>(data));
}

/// Dispatch PutLocalTyped based on DataType.
void PutLocalRawArray(adios2::IO& io,
                      adios2::Engine& engine,
                      const std::string& name,
                      const RawArray& raw)
{
  const size_t nComp = raw.NumComponents > 1 ? static_cast<size_t>(raw.NumComponents) : 1;
  switch (raw.Type)
  {
    case DataType::Float32:
      PutLocalTyped<float>(io, engine, name, raw.Data.get(), raw.NumValues, nComp);
      break;
    case DataType::Float64:
      PutLocalTyped<double>(io, engine, name, raw.Data.get(), raw.NumValues, nComp);
      break;
    case DataType::Int8:
      PutLocalTyped<int8_t>(io, engine, name, raw.Data.get(), raw.NumValues, nComp);
      break;
    case DataType::Int16:
      PutLocalTyped<int16_t>(io, engine, name, raw.Data.get(), raw.NumValues, nComp);
      break;
    case DataType::Int32:
      PutLocalTyped<int32_t>(io, engine, name, raw.Data.get(), raw.NumValues, nComp);
      break;
    case DataType::Int64:
      PutLocalTyped<int64_t>(io, engine, name, raw.Data.get(), raw.NumValues, nComp);
      break;
    case DataType::UInt8:
      PutLocalTyped<uint8_t>(io, engine, name, raw.Data.get(), raw.NumValues, nComp);
      break;
    case DataType::UInt16:
      PutLocalTyped<uint16_t>(io, engine, name, raw.Data.get(), raw.NumValues, nComp);
      break;
    case DataType::UInt32:
      PutLocalTyped<uint32_t>(io, engine, name, raw.Data.get(), raw.NumValues, nComp);
      break;
    case DataType::UInt64:
      PutLocalTyped<uint64_t>(io, engine, name, raw.Data.get(), raw.NumValues, nComp);
      break;
    default:
      throw std::runtime_error("PutLocalRawArray: unsupported DataType");
  }
}

/// Define a local (one-block-per-Put) ADIOS2 variable of the given
/// DataType. The block count is set per-Put via PutLocalRawArray, so
/// the count passed at definition time is just a placeholder.
void DefineLocalVariable(adios2::IO& io, const std::string& name, DataType type)
{
  const std::vector<size_t> empty{};
  const std::vector<size_t> one{ 1 };
  switch (type)
  {
    case DataType::Float32:
      io.DefineVariable<float>(name, empty, empty, one);
      break;
    case DataType::Float64:
      io.DefineVariable<double>(name, empty, empty, one);
      break;
    case DataType::Int8:
      io.DefineVariable<int8_t>(name, empty, empty, one);
      break;
    case DataType::Int16:
      io.DefineVariable<int16_t>(name, empty, empty, one);
      break;
    case DataType::Int32:
      io.DefineVariable<int32_t>(name, empty, empty, one);
      break;
    case DataType::Int64:
      io.DefineVariable<int64_t>(name, empty, empty, one);
      break;
    case DataType::UInt8:
      io.DefineVariable<uint8_t>(name, empty, empty, one);
      break;
    case DataType::UInt16:
      io.DefineVariable<uint16_t>(name, empty, empty, one);
      break;
    case DataType::UInt32:
      io.DefineVariable<uint32_t>(name, empty, empty, one);
      break;
    case DataType::UInt64:
      io.DefineVariable<uint64_t>(name, empty, empty, one);
      break;
    default:
      throw std::runtime_error("DefineLocalVariable: unsupported DataType");
  }
}

/// Build the full ADIOS2 variable name "group/arrayName" that goes into
/// the per-(attr,ct) /values and /connectivity ADIOS2 attributes. If
/// the group is empty the name is just the array name; if the array
/// name is also empty the variable is omitted from the schema.
std::string CellGridVariableName(const std::string& group, const std::string& name)
{
  if (group.empty())
  {
    return name;
  }
  return group + "/" + name;
}

/// Validate that a PartitionInfo has the required fields for its type combination.
void ValidatePartitionInfo(const PartitionInfo& info)
{
  if (info.Coordinates == PartitionInfo::CoordType::Uniform &&
      info.Cells == PartitionInfo::CellType::Structured)
  {
    if (info.Dims[0] == 0 || info.Dims[1] == 0 || info.Dims[2] == 0)
    {
      throw std::runtime_error("Uniform/Structured partition requires non-zero Dims.");
    }
  }
  else if (info.Coordinates == PartitionInfo::CoordType::Rectilinear &&
           info.Cells == PartitionInfo::CellType::Structured)
  {
    if (info.XCoords.NumValues == 0 || info.YCoords.NumValues == 0 || info.ZCoords.NumValues == 0)
    {
      throw std::runtime_error(
        "Rectilinear/Structured partition requires non-empty XCoords, YCoords, ZCoords.");
    }
  }
  else if (info.Coordinates == PartitionInfo::CoordType::Explicit &&
           info.Cells == PartitionInfo::CellType::Structured)
  {
    if (info.ExplicitCoords.NumValues == 0)
    {
      throw std::runtime_error("Explicit/Structured partition requires non-empty ExplicitCoords.");
    }
    if (info.Dims[0] == 0 || info.Dims[1] == 0 || info.Dims[2] == 0)
    {
      throw std::runtime_error("Explicit/Structured partition requires non-zero Dims.");
    }
  }
  else if (info.Coordinates == PartitionInfo::CoordType::Explicit &&
           info.Cells == PartitionInfo::CellType::SingleType)
  {
    if (info.ExplicitCoords.NumValues == 0)
    {
      throw std::runtime_error("Explicit/SingleType partition requires non-empty ExplicitCoords.");
    }
    if (info.SingleTypeConnectivity.NumValues == 0)
    {
      throw std::runtime_error(
        "Explicit/SingleType partition requires non-empty SingleTypeConnectivity.");
    }
    if (info.VertsPerCell <= 0)
    {
      throw std::runtime_error("Explicit/SingleType partition requires VertsPerCell > 0.");
    }
  }
  else if (info.Coordinates == PartitionInfo::CoordType::Explicit &&
           info.Cells == PartitionInfo::CellType::Explicit)
  {
    if (info.ExplicitCoords.NumValues == 0)
    {
      throw std::runtime_error("Explicit/Explicit partition requires non-empty ExplicitCoords.");
    }
    if (info.ExplicitCellTypes.NumValues == 0)
    {
      throw std::runtime_error("Explicit/Explicit partition requires non-empty ExplicitCellTypes.");
    }
    if (info.ExplicitNumVerts.NumValues == 0)
    {
      throw std::runtime_error("Explicit/Explicit partition requires non-empty ExplicitNumVerts.");
    }
    if (info.ExplicitConnectivity.NumValues == 0)
    {
      throw std::runtime_error(
        "Explicit/Explicit partition requires non-empty ExplicitConnectivity.");
    }
  }
}

/// Generate a Fides schema JSON from PartitionInfo metadata.
std::string GenerateSchemaJSON(const PartitionInfo& info,
                               const std::set<std::string>& fieldsToWrite,
                               size_t totalNumberOfBlocks,
                               bool hasTime = false,
                               bool writeAll = true)
{
  rapidjson::Document doc;
  doc.SetObject();
  auto& alloc = doc.GetAllocator();

  // The schema must be wrapped: { "ModelName": { ... } }
  rapidjson::Value inner(rapidjson::kObjectType);

  // data_sources
  rapidjson::Value dsArr(rapidjson::kArrayType);
  rapidjson::Value dsObj(rapidjson::kObjectType);
  dsObj.AddMember("name", "source", alloc);
  dsObj.AddMember("filename_mode", "input", alloc);
  dsArr.PushBack(dsObj, alloc);
  inner.AddMember("data_sources", dsArr, alloc);

  // step_information. When the writer's been told a time via SetCurrentTime
  // we name the per-step "time" variable so the reader populates TIME_ARRAY
  // (and consumers like ParaView surface real time values, not 0..N-1).
  rapidjson::Value stepInfo(rapidjson::kObjectType);
  stepInfo.AddMember("data_source", "source", alloc);
  if (hasTime)
  {
    stepInfo.AddMember("variable", "time", alloc);
  }
  inner.AddMember("step_information", stepInfo, alloc);

  // number_of_blocks
  inner.AddMember("number_of_blocks", static_cast<unsigned int>(totalNumberOfBlocks), alloc);

  std::string modelType = info.GetDataModelTypeString();

  // coordinate_system
  if (info.Coordinates == PartitionInfo::CoordType::Uniform)
  {
    fides::predefined::CreateArrayUniformPointCoordinates(
      alloc, inner, "dims", "origin", "spacing");
  }
  else if (info.Coordinates == PartitionInfo::CoordType::Rectilinear)
  {
    fides::predefined::CreateArrayRectilinearPointCoordinates(
      alloc, inner, "x_array", "y_array", "z_array");
  }
  else
  {
    fides::predefined::CreateArrayUnstructuredPointCoordinates(alloc, inner, "coordinates");
  }

  // cell_set
  if (info.Cells == PartitionInfo::CellType::Structured)
  {
    fides::predefined::CreateStructuredCellset(alloc, inner, "dims");
  }
  else if (info.Cells == PartitionInfo::CellType::SingleType)
  {
    std::string cellStr = ConvertCellShapeToString(info.SingleCellShape);
    fides::predefined::CreateUnstructuredSingleTypeCellset(alloc, inner, "connectivity", cellStr);
  }
  else if (info.Cells == PartitionInfo::CellType::PolyData)
  {
    rapidjson::Value cellSetObj(rapidjson::kObjectType);
    cellSetObj.AddMember("cell_set_type", "polydata", alloc);

    // Emit a role object only when the corresponding _offsets / _connectivity
    // variables exist on disk. Empty roles are simply absent from the
    // schema; the reader treats absence as "no cells of that kind".
    auto addRole = [&](const char* roleKey, bool present) {
      if (!present)
      {
        return;
      }
      rapidjson::Value roleObj(rapidjson::kObjectType);
      rapidjson::Value offsetsObj(rapidjson::kObjectType);
      std::string offName = std::string(roleKey) + "_offsets";
      fides::predefined::CreateArrayBasic(alloc, offsetsObj, "source", offName);
      roleObj.AddMember("offsets", offsetsObj, alloc);
      rapidjson::Value connObj(rapidjson::kObjectType);
      std::string connName = std::string(roleKey) + "_connectivity";
      fides::predefined::CreateArrayBasic(alloc, connObj, "source", connName);
      roleObj.AddMember("connectivity", connObj, alloc);
      rapidjson::Value roleKeyVal;
      roleKeyVal.SetString(roleKey, alloc);
      cellSetObj.AddMember(roleKeyVal, roleObj, alloc);
    };
    addRole("verts", info.PolyDataVertsOffsets.IsValid());
    addRole("lines", info.PolyDataLinesOffsets.IsValid());
    addRole("polys", info.PolyDataPolysOffsets.IsValid());
    addRole("strips", info.PolyDataStripsOffsets.IsValid());

    inner.AddMember("cell_set", cellSetObj, alloc);
  }
  else
  {
    // Explicit cell set
    rapidjson::Value cellSetObj(rapidjson::kObjectType);
    cellSetObj.AddMember("cell_set_type", "explicit", alloc);

    // cell_types must be an Array object
    rapidjson::Value cellTypesObj(rapidjson::kObjectType);
    fides::predefined::CreateArrayBasic(alloc, cellTypesObj, "source", "cell_types");
    cellSetObj.AddMember("cell_types", cellTypesObj, alloc);

    // number_of_vertices must be an Array object
    rapidjson::Value numVertsObj(rapidjson::kObjectType);
    fides::predefined::CreateArrayBasic(alloc, numVertsObj, "source", "num_verts");
    cellSetObj.AddMember("number_of_vertices", numVertsObj, alloc);

    // connectivity must be an Array object
    rapidjson::Value connObj(rapidjson::kObjectType);
    fides::predefined::CreateArrayBasic(alloc, connObj, "source", "connectivity");
    cellSetObj.AddMember("connectivity", connObj, alloc);

    inner.AddMember("cell_set", cellSetObj, alloc);
  }

  // fields
  rapidjson::Value fieldsArr(rapidjson::kArrayType);
  for (const auto& field : info.Fields)
  {
    if (!writeAll && fieldsToWrite.find(field.Name) == fieldsToWrite.end())
    {
      continue;
    }

    rapidjson::Value fieldObj(rapidjson::kObjectType);
    fieldObj.AddMember("name", fides::predefined::SetString(alloc, field.Name), alloc);

    std::string assocStr = (field.Association == FieldAssociation::Points) ? "points" : "cell_set";
    fieldObj.AddMember("association", fides::predefined::SetString(alloc, assocStr), alloc);

    rapidjson::Value arrObj(rapidjson::kObjectType);
    fides::predefined::CreateArrayBasic(alloc,
                                        arrObj,
                                        "source",
                                        field.Name,
                                        false,
                                        "basic",
                                        (field.Data.NumComponents > 1) ? "true" : "");
    fieldObj.AddMember("array", arrObj, alloc);
    fieldsArr.PushBack(fieldObj, alloc);
  }
  inner.AddMember("fields", fieldsArr, alloc);

  if (hasTime)
  {
    rapidjson::Value timeObj(rapidjson::kObjectType);
    fides::predefined::CreateArrayBasic(alloc, timeObj, "source", "time");
    inner.AddMember("step_variable", timeObj, alloc);
  }

  doc.AddMember("Fides_Generated", inner, alloc);

  // Serialize
  rapidjson::StringBuffer buf;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buf);
  doc.Accept(writer);
  return buf.GetString();
}

/// Build the cell_grid schema. The writer doesn't know which attribute
/// the reader will want to mark static, so it sets "static": true on
/// the shape attribute (the conventional case) and leaves the others
/// dynamic. Structural metadata (cell types, function spaces, array
/// names) is recovered by the reader from the ADIOS2 attributes
/// written alongside this schema.
std::string GenerateCellGridSchemaJSON(const CellGridPartitionInfo& info, bool hasTime = false)
{
  rapidjson::Document doc;
  doc.SetObject();
  auto& alloc = doc.GetAllocator();

  rapidjson::Value inner(rapidjson::kObjectType);

  // data_sources
  rapidjson::Value dsArr(rapidjson::kArrayType);
  rapidjson::Value dsObj(rapidjson::kObjectType);
  dsObj.AddMember("name", "source", alloc);
  dsObj.AddMember("filename_mode", "input", alloc);
  dsArr.PushBack(dsObj, alloc);
  inner.AddMember("data_sources", dsArr, alloc);

  // step_information / step_variable -- only when SetCurrentTime was ever
  // called on the writer. Without them the reader can't surface ADIOS2
  // steps as VTK time steps even though they're present in the file.
  // The "variable" field is required so the reader knows which ADIOS
  // variable holds the per-step time values; "step_variable" is the
  // companion array-shape block consumed by the same lookup path.
  if (hasTime)
  {
    rapidjson::Value stepInfo(rapidjson::kObjectType);
    stepInfo.AddMember("data_source", "source", alloc);
    stepInfo.AddMember("variable", "time", alloc);
    inner.AddMember("step_information", stepInfo, alloc);
  }

  // cell_attributes
  rapidjson::Value attrsArr(rapidjson::kArrayType);
  for (const auto& attr : info.Attributes)
  {
    rapidjson::Value attrObj(rapidjson::kObjectType);
    attrObj.AddMember("name", fides::predefined::SetString(alloc, attr.Name), alloc);
    attrObj.AddMember("data_source", "source", alloc);
    if (attr.IsShape)
    {
      attrObj.AddMember("static", true, alloc);
    }
    attrsArr.PushBack(attrObj, alloc);
  }
  inner.AddMember("cell_attributes", attrsArr, alloc);

  if (hasTime)
  {
    rapidjson::Value timeObj(rapidjson::kObjectType);
    fides::predefined::CreateArrayBasic(alloc, timeObj, "source", "time");
    inner.AddMember("step_variable", timeObj, alloc);
  }

  doc.AddMember("cell_grid", inner, alloc);

  rapidjson::StringBuffer buf;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buf);
  doc.Accept(writer);
  return buf.GetString();
}

/// Accessor for one of vtkPolyData's four cell-array roles on a
/// PartitionInfo, by role index 0..3 (verts, lines, polys, strips).
/// Returns a (offsets, connectivity) pair of references. Both members
/// of the pair will be invalid (default-constructed) when the role
/// has no cells in this partition.
struct PolyDataRoleRef
{
  const RawArray& Offsets;
  const RawArray& Connectivity;
};
PolyDataRoleRef GetPolyDataRole(const PartitionInfo& p, int roleIdx)
{
  switch (roleIdx)
  {
    case 0:
      return { p.PolyDataVertsOffsets, p.PolyDataVertsConnectivity };
    case 1:
      return { p.PolyDataLinesOffsets, p.PolyDataLinesConnectivity };
    case 2:
      return { p.PolyDataPolysOffsets, p.PolyDataPolysConnectivity };
    case 3:
      return { p.PolyDataStripsOffsets, p.PolyDataStripsConnectivity };
    default:
      throw std::runtime_error("GetPolyDataRole: invalid role index");
  }
}

} // end anon namespace

struct FidesWriter::Impl
{
  Impl(const std::string& outputFile, const std::string& outputMode)
    : OutputFile(outputFile)
    , OutputMode(outputMode)
#if FIDES_USE_MPI
    , Comm(MPI_COMM_NULL)
#endif
  {
    FIDES_SAFE_MPI(this->Comm, MPI_Comm_rank(this->Comm, &this->Rank));
    FIDES_SAFE_MPI(this->Comm, MPI_Comm_size(this->Comm, &this->NumRanks));
  }

#if FIDES_USE_MPI
  Impl(const std::string& outputFile, MPI_Comm comm, const std::string& outputMode)
    : OutputFile(outputFile)
    , OutputMode(outputMode)
    , Comm(comm)
  {
    FIDES_SAFE_MPI(this->Comm, MPI_Comm_rank(this->Comm, &this->Rank));
    FIDES_SAFE_MPI(this->Comm, MPI_Comm_size(this->Comm, &this->NumRanks));
  }
#endif

  void EnsureEngineOpen()
  {
    if (this->EngineOpen)
    {
      return;
    }

    // Create the ADIOS instance once (deferred so SetAdiosConfigFile can be called first)
    if (!this->AdiosCreated)
    {
      if (!this->AdiosConfigFile.empty())
      {
#if FIDES_USE_MPI
        this->Adios = adios2::ADIOS(this->AdiosConfigFile, this->Comm);
#else
        this->Adios = adios2::ADIOS(this->AdiosConfigFile);
#endif
      }
      else
      {
#if FIDES_USE_MPI
        this->Adios = adios2::ADIOS(this->Comm);
#else
        this->Adios = adios2::ADIOS();
#endif
      }
      this->AdiosCreated = true;
    }

    if (!this->AdiosConfigFile.empty())
    {
      this->IO = this->Adios.DeclareIO("fides-write-io");
    }
    else if (!this->OutputMode.empty())
    {
      this->IO = this->Adios.DeclareIO("fides-write-io");
      this->IO.SetEngine(this->OutputMode);
    }
    else
    {
      throw std::runtime_error("Error: No output mode or adios config file provided.");
    }

    if (!this->EngineParams.empty())
    {
      this->IO.SetParameters(this->EngineParams);
    }

    this->Engine = this->IO.Open(this->OutputFile, adios2::Mode::Write);
    this->EngineOpen = true;
  }

  void ComputeGlobalBlockInfo(const std::vector<PartitionInfo>& partitions)
  {
    this->NumberOfDataSets = partitions.size();

    this->DataSetsPerRank.clear();
    this->DataSetsPerRank.resize(static_cast<size_t>(this->NumRanks), 0);
    this->DataSetsPerRank[static_cast<size_t>(this->Rank)] = this->NumberOfDataSets;

    FIDES_SAFE_MPI(this->Comm,
                   MPI_Allreduce(MPI_IN_PLACE,
                                 this->DataSetsPerRank.data(),
                                 this->NumRanks,
                                 FIDES_MPI_SIZE_T,
                                 MPI_SUM,
                                 this->Comm));

    this->TotalNumberOfDataSets =
      std::accumulate(this->DataSetsPerRank.begin(), this->DataSetsPerRank.end(), size_t{ 0 });

    this->DataSetOffset = 0;
    for (size_t i = 0; i < static_cast<size_t>(this->Rank); i++)
    {
      this->DataSetOffset += this->DataSetsPerRank[i];
    }

    // Compute point and cell offsets
    std::vector<size_t> numPoints(static_cast<size_t>(this->NumRanks), 0);
    std::vector<size_t> numCells(static_cast<size_t>(this->NumRanks), 0);

    for (const auto& p : partitions)
    {
      numPoints[static_cast<size_t>(this->Rank)] += p.NumberOfPoints();
      numCells[static_cast<size_t>(this->Rank)] += p.NumberOfCells();
    }

    FIDES_SAFE_MPI(
      this->Comm,
      MPI_Allreduce(
        MPI_IN_PLACE, numPoints.data(), this->NumRanks, FIDES_MPI_SIZE_T, MPI_SUM, this->Comm));
    FIDES_SAFE_MPI(
      this->Comm,
      MPI_Allreduce(
        MPI_IN_PLACE, numCells.data(), this->NumRanks, FIDES_MPI_SIZE_T, MPI_SUM, this->Comm));

    this->TotalNumberOfPoints = std::accumulate(numPoints.begin(), numPoints.end(), size_t{ 0 });
    this->TotalNumberOfCells = std::accumulate(numCells.begin(), numCells.end(), size_t{ 0 });

    this->DataSetPointsOffset = 0;
    this->DataSetCellsOffset = 0;
    for (size_t i = 0; i < static_cast<size_t>(this->Rank); i++)
    {
      this->DataSetPointsOffset += numPoints[i];
      this->DataSetCellsOffset += numCells[i];
    }

    // Type-specific global info
    if (!partitions.empty())
    {
      this->GlobalCoordType = partitions[0].Coordinates;
      this->GlobalCellType = partitions[0].Cells;
    }

#if FIDES_USE_MPI
    // Validate that all ranks agree on coord/cell types.
    // Empty ranks (no partitions) use sentinel values so they don't
    // affect the MIN/MAX comparison, then adopt the consensus type.
    {
      bool hasData = !partitions.empty();
      // Empty ranks use INT_MAX for MIN and INT_MIN for MAX so they
      // don't affect either reduction among non-empty ranks.
      int localCoordForMin = hasData ? static_cast<int>(this->GlobalCoordType) : INT_MAX;
      int localCellForMin = hasData ? static_cast<int>(this->GlobalCellType) : INT_MAX;
      int localCoordForMax = hasData ? static_cast<int>(this->GlobalCoordType) : INT_MIN;
      int localCellForMax = hasData ? static_cast<int>(this->GlobalCellType) : INT_MIN;

      int minCoord = localCoordForMin;
      int maxCoord = localCoordForMax;
      int minCell = localCellForMin;
      int maxCell = localCellForMax;

      FIDES_SAFE_MPI(this->Comm,
                     MPI_Allreduce(&localCoordForMin, &minCoord, 1, MPI_INT, MPI_MIN, this->Comm));
      FIDES_SAFE_MPI(this->Comm,
                     MPI_Allreduce(&localCoordForMax, &maxCoord, 1, MPI_INT, MPI_MAX, this->Comm));
      FIDES_SAFE_MPI(this->Comm,
                     MPI_Allreduce(&localCellForMin, &minCell, 1, MPI_INT, MPI_MIN, this->Comm));
      FIDES_SAFE_MPI(this->Comm,
                     MPI_Allreduce(&localCellForMax, &maxCell, 1, MPI_INT, MPI_MAX, this->Comm));

      if (minCoord != maxCoord || minCell != maxCell)
      {
        throw std::runtime_error("All ranks must have the same CoordType and CellType.");
      }

      // Empty ranks adopt the consensus type from ranks that have data
      if (!hasData && minCoord != INT_MAX)
      {
        this->GlobalCoordType = static_cast<PartitionInfo::CoordType>(minCoord);
        this->GlobalCellType = static_cast<PartitionInfo::CellType>(minCell);
      }
    }
#endif

    // For rectilinear: compute per-axis coordinate totals
    if (this->GlobalCoordType == PartitionInfo::CoordType::Rectilinear)
    {
      this->ComputeRectilinearGlobalInfo(partitions);
    }
    // For single-type unstructured: compute connectivity totals
    else if (this->GlobalCellType == PartitionInfo::CellType::SingleType)
    {
      this->ComputeSingleTypeGlobalInfo(partitions);
    }
    // For explicit unstructured: compute connectivity totals
    else if (this->GlobalCellType == PartitionInfo::CellType::Explicit)
    {
      this->ComputeExplicitGlobalInfo(partitions);
    }
    else if (this->GlobalCellType == PartitionInfo::CellType::PolyData)
    {
      this->ComputePolyDataGlobalInfo(partitions);
    }
    // For explicit coords + structured cells (not uniform, not rectilinear)
    else if (this->GlobalCoordType == PartitionInfo::CoordType::Explicit &&
             this->GlobalCellType == PartitionInfo::CellType::Structured)
    {
      // Coordinates offset same as points offset, already computed
      this->TotalNumberOfCoords = this->TotalNumberOfPoints;
      this->CoordOffset = this->DataSetPointsOffset;
    }
  }

  void ComputeRectilinearGlobalInfo(const std::vector<PartitionInfo>& partitions)
  {
    std::vector<size_t> numCoords(static_cast<size_t>(this->NumRanks) * 3, 0);

    this->NumXCoords = 0;
    this->NumYCoords = 0;
    this->NumZCoords = 0;
    for (const auto& p : partitions)
    {
      this->NumXCoords += p.XCoords.NumValues;
      this->NumYCoords += p.YCoords.NumValues;
      this->NumZCoords += p.ZCoords.NumValues;
    }
    numCoords[static_cast<size_t>(this->Rank) * 3 + 0] = this->NumXCoords;
    numCoords[static_cast<size_t>(this->Rank) * 3 + 1] = this->NumYCoords;
    numCoords[static_cast<size_t>(this->Rank) * 3 + 2] = this->NumZCoords;

    FIDES_SAFE_MPI(
      this->Comm,
      MPI_Allreduce(
        MPI_IN_PLACE, numCoords.data(), this->NumRanks * 3, FIDES_MPI_SIZE_T, MPI_SUM, this->Comm));

    this->TotalXCoords = 0;
    this->TotalYCoords = 0;
    this->TotalZCoords = 0;
    for (int i = 0; i < this->NumRanks; i++)
    {
      this->TotalXCoords += numCoords[static_cast<size_t>(i) * 3 + 0];
      this->TotalYCoords += numCoords[static_cast<size_t>(i) * 3 + 1];
      this->TotalZCoords += numCoords[static_cast<size_t>(i) * 3 + 2];
    }

    this->XCoordsOffset = 0;
    this->YCoordsOffset = 0;
    this->ZCoordsOffset = 0;
    for (int i = 0; i < this->Rank; i++)
    {
      this->XCoordsOffset += numCoords[static_cast<size_t>(i) * 3 + 0];
      this->YCoordsOffset += numCoords[static_cast<size_t>(i) * 3 + 1];
      this->ZCoordsOffset += numCoords[static_cast<size_t>(i) * 3 + 2];
    }
  }

  void ComputeSingleTypeGlobalInfo(const std::vector<PartitionInfo>& partitions)
  {
    std::vector<size_t> numCoords(static_cast<size_t>(this->NumRanks), 0);
    std::vector<size_t> numConns(static_cast<size_t>(this->NumRanks), 0);
    std::vector<size_t> numPtsInCell(static_cast<size_t>(this->NumRanks), 0);
    std::vector<size_t> cellShape(static_cast<size_t>(this->NumRanks), 0);

    size_t localCoords = 0;
    size_t localConns = 0;
    this->GlobalVertsPerCell = 0;

    for (const auto& p : partitions)
    {
      localCoords += p.NumberOfPoints();
      localConns += p.SingleTypeConnectivity.NumValues;
      if (p.NumberOfCells() > 0 && this->GlobalVertsPerCell == 0)
      {
        this->GlobalVertsPerCell = static_cast<size_t>(p.VertsPerCell);
        this->GlobalCellShape = p.SingleCellShape;
      }
    }

    numCoords[static_cast<size_t>(this->Rank)] = localCoords;
    numConns[static_cast<size_t>(this->Rank)] = localConns;
    numPtsInCell[static_cast<size_t>(this->Rank)] = this->GlobalVertsPerCell;
    cellShape[static_cast<size_t>(this->Rank)] = static_cast<size_t>(this->GlobalCellShape);

    FIDES_SAFE_MPI(
      this->Comm,
      MPI_Allreduce(
        MPI_IN_PLACE, numCoords.data(), this->NumRanks, FIDES_MPI_SIZE_T, MPI_SUM, this->Comm));
    FIDES_SAFE_MPI(
      this->Comm,
      MPI_Allreduce(
        MPI_IN_PLACE, numConns.data(), this->NumRanks, FIDES_MPI_SIZE_T, MPI_SUM, this->Comm));
    FIDES_SAFE_MPI(
      this->Comm,
      MPI_Allreduce(
        MPI_IN_PLACE, numPtsInCell.data(), this->NumRanks, FIDES_MPI_SIZE_T, MPI_SUM, this->Comm));
    FIDES_SAFE_MPI(
      this->Comm,
      MPI_Allreduce(
        MPI_IN_PLACE, cellShape.data(), this->NumRanks, FIDES_MPI_SIZE_T, MPI_SUM, this->Comm));

    this->TotalNumberOfCoords = 0;
    this->TotalNumberOfConnIds = 0;
    for (int i = 0; i < this->NumRanks; i++)
    {
      this->TotalNumberOfCoords += numCoords[static_cast<size_t>(i)];
      this->TotalNumberOfConnIds += numConns[static_cast<size_t>(i)];
      // Pick up verts/cell from a rank that has cells
      if (numConns[static_cast<size_t>(i)] > 0 && this->GlobalVertsPerCell == 0)
      {
        this->GlobalVertsPerCell = numPtsInCell[static_cast<size_t>(i)];
        this->GlobalCellShape = static_cast<CellShape>(cellShape[static_cast<size_t>(i)]);
      }
    }

    this->CoordOffset = 0;
    this->ConnOffset = 0;
    for (int i = 0; i < this->Rank; i++)
    {
      this->CoordOffset += numCoords[static_cast<size_t>(i)];
      this->ConnOffset += numConns[static_cast<size_t>(i)];
    }
  }

  void ComputeExplicitGlobalInfo(const std::vector<PartitionInfo>& partitions)
  {
    std::vector<size_t> numCoords(static_cast<size_t>(this->NumRanks), 0);
    std::vector<size_t> numCells(static_cast<size_t>(this->NumRanks), 0);
    std::vector<size_t> numConns(static_cast<size_t>(this->NumRanks), 0);

    size_t localCoords = 0;
    size_t localCells = 0;
    size_t localConns = 0;
    for (const auto& p : partitions)
    {
      localCoords += p.NumberOfPoints();
      localCells += p.NumberOfCells();
      localConns += p.ExplicitConnectivity.NumValues;
    }

    numCoords[static_cast<size_t>(this->Rank)] = localCoords;
    numCells[static_cast<size_t>(this->Rank)] = localCells;
    numConns[static_cast<size_t>(this->Rank)] = localConns;

    FIDES_SAFE_MPI(
      this->Comm,
      MPI_Allreduce(
        MPI_IN_PLACE, numCoords.data(), this->NumRanks, FIDES_MPI_SIZE_T, MPI_SUM, this->Comm));
    FIDES_SAFE_MPI(
      this->Comm,
      MPI_Allreduce(
        MPI_IN_PLACE, numCells.data(), this->NumRanks, FIDES_MPI_SIZE_T, MPI_SUM, this->Comm));
    FIDES_SAFE_MPI(
      this->Comm,
      MPI_Allreduce(
        MPI_IN_PLACE, numConns.data(), this->NumRanks, FIDES_MPI_SIZE_T, MPI_SUM, this->Comm));

    this->TotalNumberOfCoords = 0;
    this->TotalExplicitCells = 0;
    this->TotalExplicitConns = 0;
    for (int i = 0; i < this->NumRanks; i++)
    {
      this->TotalNumberOfCoords += numCoords[static_cast<size_t>(i)];
      this->TotalExplicitCells += numCells[static_cast<size_t>(i)];
      this->TotalExplicitConns += numConns[static_cast<size_t>(i)];
    }

    this->CoordOffset = 0;
    this->CellOffset = 0;
    this->ConnOffset = 0;
    for (int i = 0; i < this->Rank; i++)
    {
      this->CoordOffset += numCoords[static_cast<size_t>(i)];
      this->CellOffset += numCells[static_cast<size_t>(i)];
      this->ConnOffset += numConns[static_cast<size_t>(i)];
    }
  }

  void ComputePolyDataGlobalInfo(const std::vector<PartitionInfo>& partitions)
  {
    // Coords totals (same shape as the other explicit-coords paths).
    std::vector<size_t> numCoords(static_cast<size_t>(this->NumRanks), 0);
    size_t localCoords = 0;
    for (const auto& p : partitions)
    {
      localCoords += p.NumberOfPoints();
    }
    numCoords[static_cast<size_t>(this->Rank)] = localCoords;
#if FIDES_USE_MPI
    MPI_Allreduce(
      MPI_IN_PLACE, numCoords.data(), this->NumRanks, FIDES_MPI_SIZE_T, MPI_SUM, this->Comm);
#endif
    this->TotalNumberOfCoords = 0;
    for (int i = 0; i < this->NumRanks; i++)
    {
      this->TotalNumberOfCoords += numCoords[static_cast<size_t>(i)];
    }
    this->CoordOffset = 0;
    for (int i = 0; i < this->Rank; i++)
    {
      this->CoordOffset += numCoords[static_cast<size_t>(i)];
    }

    // Per-role stream totals. Two streams per role (offsets, connectivity).
    // Pack into one 8-wide MPI exchange for compactness.
    constexpr int kRoles = 4;
    std::vector<size_t> rankCounts(static_cast<size_t>(this->NumRanks) * kRoles * 2, 0);
    for (int r = 0; r < kRoles; r++)
    {
      auto& role = this->PolyDataRoles[static_cast<size_t>(r)];
      role.LocalOffsets = 0;
      role.LocalConn = 0;
      for (const auto& p : partitions)
      {
        auto ref = GetPolyDataRole(p, r);
        role.LocalOffsets += ref.Offsets.NumValues;
        role.LocalConn += ref.Connectivity.NumValues;
        if (ref.Offsets.IsValid())
        {
          role.OffsetsType = ref.Offsets.Type;
        }
        if (ref.Connectivity.IsValid())
        {
          role.ConnType = ref.Connectivity.Type;
        }
      }
      rankCounts[static_cast<size_t>(this->Rank) * kRoles * 2 + r * 2 + 0] = role.LocalOffsets;
      rankCounts[static_cast<size_t>(this->Rank) * kRoles * 2 + r * 2 + 1] = role.LocalConn;
    }
#if FIDES_USE_MPI
    MPI_Allreduce(MPI_IN_PLACE,
                  rankCounts.data(),
                  this->NumRanks * kRoles * 2,
                  FIDES_MPI_SIZE_T,
                  MPI_SUM,
                  this->Comm);
#endif
    for (int r = 0; r < kRoles; r++)
    {
      auto& role = this->PolyDataRoles[static_cast<size_t>(r)];
      role.TotalOffsets = 0;
      role.TotalConn = 0;
      role.OffsetsRankStart = 0;
      role.ConnRankStart = 0;
      for (int i = 0; i < this->NumRanks; i++)
      {
        const size_t off = rankCounts[static_cast<size_t>(i) * kRoles * 2 + r * 2 + 0];
        const size_t con = rankCounts[static_cast<size_t>(i) * kRoles * 2 + r * 2 + 1];
        role.TotalOffsets += off;
        role.TotalConn += con;
        if (i < this->Rank)
        {
          role.OffsetsRankStart += off;
          role.ConnRankStart += con;
        }
      }
    }
  }

  void DefineVariables(const std::vector<PartitionInfo>& partitions)
  {
    if (partitions.empty() && this->TotalNumberOfDataSets == 0)
    {
      return;
    }

    // Get a representative partition (from any rank that has one)
    const PartitionInfo* rep = partitions.empty() ? nullptr : partitions.data();

    if (this->GlobalCoordType == PartitionInfo::CoordType::Uniform)
    {
      this->DefineUniformVariables();
    }
    else if (this->GlobalCoordType == PartitionInfo::CoordType::Rectilinear)
    {
      this->DefineRectilinearVariables(rep);
    }
    else if (this->GlobalCellType == PartitionInfo::CellType::Structured)
    {
      this->DefineStructuredExplicitVariables(rep);
    }
    else if (this->GlobalCellType == PartitionInfo::CellType::SingleType)
    {
      this->DefineSingleTypeVariables(rep);
    }
    else if (this->GlobalCellType == PartitionInfo::CellType::Explicit)
    {
      this->DefineExplicitVariables(rep);
    }
    else if (this->GlobalCellType == PartitionInfo::CellType::PolyData)
    {
      this->DefinePolyDataVariables(rep);
    }

    // Define field variables
    this->DefineFieldVariables(partitions);
  }

  void DefineUniformVariables()
  {
    std::vector<size_t> shape = { 3 * this->TotalNumberOfDataSets };
    std::vector<size_t> offset = { 3 * this->DataSetOffset };
    std::vector<size_t> size = { 3 * this->NumberOfDataSets };

    this->IO.DefineVariable<size_t>("dims", shape, offset, size);
    this->IO.DefineVariable<double>("origin", shape, offset, size);
    this->IO.DefineVariable<double>("spacing", shape, offset, size);
  }

  void DefineStructuredExplicitVariables(const PartitionInfo* rep)
  {
    // Coordinates: Nx3
    DataType coordType =
      (rep && rep->ExplicitCoords.IsValid()) ? rep->ExplicitCoords.Type : DataType::Float64;
    std::vector<size_t> shape = { this->TotalNumberOfCoords, 3 };
    std::vector<size_t> offset = { this->CoordOffset, 0 };
    std::vector<size_t> count = { this->LocalPoints, 3 };
    DefineVariable(this->IO, "coordinates", coordType, shape, offset, count);

    // Dims: 3 values per block
    shape = { 3 * this->TotalNumberOfDataSets };
    offset = { 3 * this->DataSetOffset };
    count = { 3 * this->NumberOfDataSets };
    this->IO.DefineVariable<size_t>("dims", shape, offset, count);
  }

  void DefineRectilinearVariables(const PartitionInfo* rep)
  {
    std::vector<size_t> shape, offset, size;

    shape = { 3 * this->TotalNumberOfDataSets };
    offset = { 3 * this->DataSetOffset };
    size = { 3 * this->NumberOfDataSets };
    this->IO.DefineVariable<size_t>("dims", shape, offset, size);

    DataType coordType = (rep && rep->XCoords.IsValid()) ? rep->XCoords.Type : DataType::Float64;

    shape = { this->TotalXCoords };
    offset = { this->XCoordsOffset };
    size = { this->NumXCoords };
    DefineVariable(this->IO, "x_array", coordType, shape, offset, size);

    shape = { this->TotalYCoords };
    offset = { this->YCoordsOffset };
    size = { this->NumYCoords };
    DefineVariable(this->IO, "y_array", coordType, shape, offset, size);

    shape = { this->TotalZCoords };
    offset = { this->ZCoordsOffset };
    size = { this->NumZCoords };
    DefineVariable(this->IO, "z_array", coordType, shape, offset, size);
  }

  void DefineSingleTypeVariables(const PartitionInfo* rep)
  {
    // Coordinates: Nx3
    DataType coordType =
      (rep && rep->ExplicitCoords.IsValid()) ? rep->ExplicitCoords.Type : DataType::Float64;
    std::vector<size_t> shape = { this->TotalNumberOfCoords, 3 };
    std::vector<size_t> offset = { this->CoordOffset, 0 };
    std::vector<size_t> count = { this->LocalPoints, 3 };
    DefineVariable(this->IO, "coordinates", coordType, shape, offset, count);

    // Connectivity
    DataType connType = (rep && rep->SingleTypeConnectivity.IsValid())
      ? rep->SingleTypeConnectivity.Type
      : DataType::Int64;
    shape = { this->TotalNumberOfConnIds };
    offset = { this->ConnOffset };
    count = { this->LocalConns };
    DefineVariable(this->IO, "connectivity", connType, shape, offset, count);
  }

  void DefineExplicitVariables(const PartitionInfo* rep)
  {
    DataType coordType =
      (rep && rep->ExplicitCoords.IsValid()) ? rep->ExplicitCoords.Type : DataType::Float64;

    std::vector<size_t> shape = { this->TotalNumberOfCoords, 3 };
    std::vector<size_t> offset = { this->CoordOffset, 0 };
    std::vector<size_t> count = { this->LocalPoints, 3 };
    DefineVariable(this->IO, "coordinates", coordType, shape, offset, count);

    this->IO.DefineVariable<uint8_t>(
      "cell_types", { this->TotalExplicitCells }, { this->CellOffset }, { this->LocalCells });
    this->IO.DefineVariable<int32_t>(
      "num_verts", { this->TotalExplicitCells }, { this->CellOffset }, { this->LocalCells });

    DataType connType = (rep && rep->ExplicitConnectivity.IsValid())
      ? rep->ExplicitConnectivity.Type
      : DataType::Int64;
    shape = { this->TotalExplicitConns };
    offset = { this->ConnOffset };
    count = { this->LocalConns };
    DefineVariable(this->IO, "connectivity", connType, shape, offset, count);
  }

  void DefinePolyDataVariables(const PartitionInfo* rep)
  {
    DataType coordType =
      (rep && rep->ExplicitCoords.IsValid()) ? rep->ExplicitCoords.Type : DataType::Float64;

    std::vector<size_t> shape = { this->TotalNumberOfCoords, 3 };
    std::vector<size_t> offset = { this->CoordOffset, 0 };
    std::vector<size_t> count = { this->LocalPoints, 3 };
    DefineVariable(this->IO, "coordinates", coordType, shape, offset, count);

    // Define two 1-D streams per role, but only when the role has any
    // cells globally; this lets readers detect "no cells of this kind"
    // by the absence of the variable.
    for (const auto& role : this->PolyDataRoles)
    {
      if (role.TotalOffsets == 0)
      {
        continue;
      }
      DefineVariable(this->IO,
                     role.Name + "_offsets",
                     role.OffsetsType,
                     { role.TotalOffsets },
                     { role.OffsetsRankStart },
                     { role.LocalOffsets });
      DefineVariable(this->IO,
                     role.Name + "_connectivity",
                     role.ConnType,
                     { role.TotalConn },
                     { role.ConnRankStart },
                     { role.LocalConn });
    }
  }

  void DefineFieldVariables(const std::vector<PartitionInfo>& partitions)
  {
    if (partitions.empty())
    {
      return;
    }

    const auto& rep = partitions[0];
    size_t localPoints = 0;
    size_t localCells = 0;
    for (const auto& p : partitions)
    {
      localPoints += p.NumberOfPoints();
      localCells += p.NumberOfCells();
    }

    this->PointFieldNames.clear();
    this->CellFieldNames.clear();

    for (const auto& field : rep.Fields)
    {
      if (!this->ShouldWriteField(field.Name))
      {
        continue;
      }

      size_t numComp = static_cast<size_t>(field.Data.NumComponents);
      std::vector<size_t> shape, offset, count;

      if (field.Association == FieldAssociation::Points)
      {
        if (numComp == 1)
        {
          shape = { this->TotalNumberOfPoints };
          offset = { this->DataSetPointsOffset };
          count = { localPoints };
        }
        else
        {
          shape = { this->TotalNumberOfPoints, numComp };
          offset = { this->DataSetPointsOffset, 0 };
          count = { localPoints, numComp };
        }
        DefineVariable(this->IO, field.Name, field.Data.Type, shape, offset, count);
        this->PointFieldNames.push_back(field.Name);
      }
      else if (field.Association == FieldAssociation::Cells)
      {
        if (numComp == 1)
        {
          shape = { this->TotalNumberOfCells };
          offset = { this->DataSetCellsOffset };
          count = { localCells };
        }
        else
        {
          shape = { this->TotalNumberOfCells, numComp };
          offset = { this->DataSetCellsOffset, 0 };
          count = { localCells, numComp };
        }
        DefineVariable(this->IO, field.Name, field.Data.Type, shape, offset, count);
        this->CellFieldNames.push_back(field.Name);
      }
    }
  }

  void WriteSchema(const std::vector<PartitionInfo>& partitions)
  {
    // Find first rank that has data
    int rankWithDS = 0;
    for (int i = 0; i < this->NumRanks; i++)
    {
      if (this->DataSetsPerRank[static_cast<size_t>(i)] > 0)
      {
        rankWithDS = i;
        break;
      }
    }

    if (this->Rank == rankWithDS && !partitions.empty())
    {
      std::string schema = GenerateSchemaJSON(partitions[0],
                                              this->FieldsToWrite,
                                              this->TotalNumberOfDataSets,
                                              this->TimeEverSet,
                                              this->WriteAll);
      this->IO.DefineAttribute<std::string>("fides/schema", schema);

      // Also write the data model type as an attribute
      std::string modelType = partitions[0].GetDataModelTypeString();
      this->IO.DefineAttribute<std::string>("Fides_Data_Model", modelType);
    }
  }

  void WriteCoordinates(const std::vector<PartitionInfo>& partitions)
  {
    if (this->GlobalCoordType == PartitionInfo::CoordType::Uniform)
    {
      this->WriteUniformCoordinates(partitions);
    }
    else if (this->GlobalCoordType == PartitionInfo::CoordType::Rectilinear)
    {
      this->WriteRectilinearCoordinates(partitions);
    }
    else
    {
      this->WriteExplicitCoordinates(partitions);
      if (this->GlobalCellType == PartitionInfo::CellType::Structured)
      {
        this->WriteStructuredDims(partitions);
      }
    }
  }

  void WriteUniformCoordinates(const std::vector<PartitionInfo>& partitions)
  {
    size_t n = partitions.size();
    this->DimsBuffer.resize(n * 3);
    this->OriginsBuffer.resize(n * 3);
    this->SpacingsBuffer.resize(n * 3);

    auto dimsVar = this->IO.InquireVariable<size_t>("dims");
    auto originVar = this->IO.InquireVariable<double>("origin");
    auto spacingVar = this->IO.InquireVariable<double>("spacing");

    std::vector<size_t> shape = { 3 * this->TotalNumberOfDataSets };
    dimsVar.SetShape(shape);
    originVar.SetShape(shape);
    spacingVar.SetShape(shape);

    for (size_t i = 0; i < n; i++)
    {
      const auto& p = partitions[i];
      for (int j = 0; j < 3; j++)
      {
        this->DimsBuffer[i * 3 + static_cast<size_t>(j)] = p.Dims[j];
        this->OriginsBuffer[i * 3 + static_cast<size_t>(j)] = p.Origin[j];
        this->SpacingsBuffer[i * 3 + static_cast<size_t>(j)] = p.Spacing[j];
      }

      adios2::Box<adios2::Dims> sel({ i * 3 + (3 * this->DataSetOffset) }, { 3 });
      dimsVar.SetSelection(sel);
      originVar.SetSelection(sel);
      spacingVar.SetSelection(sel);
      this->Engine.Put<size_t>(dimsVar, &this->DimsBuffer[i * 3]);
      this->Engine.Put<double>(originVar, &this->OriginsBuffer[i * 3]);
      this->Engine.Put<double>(spacingVar, &this->SpacingsBuffer[i * 3]);
    }
  }

  void WriteRectilinearCoordinates(const std::vector<PartitionInfo>& partitions)
  {
    size_t xcOff = this->XCoordsOffset;
    size_t ycOff = this->YCoordsOffset;
    size_t zcOff = this->ZCoordsOffset;

    size_t n = partitions.size();
    this->DimsBuffer.resize(n * 3);

    auto dimsVar = this->IO.InquireVariable<size_t>("dims");
    dimsVar.SetShape({ 3 * this->TotalNumberOfDataSets });

    for (size_t i = 0; i < n; i++)
    {
      const auto& p = partitions[i];
      size_t nx = p.XCoords.NumValues;
      size_t ny = p.YCoords.NumValues;
      size_t nz = p.ZCoords.NumValues;

      PutRawArray(
        this->IO, this->Engine, "x_array", p.XCoords, { this->TotalXCoords }, { xcOff }, { nx });
      PutRawArray(
        this->IO, this->Engine, "y_array", p.YCoords, { this->TotalYCoords }, { ycOff }, { ny });
      PutRawArray(
        this->IO, this->Engine, "z_array", p.ZCoords, { this->TotalZCoords }, { zcOff }, { nz });

      adios2::Box<adios2::Dims> sel({ i * 3 + (3 * this->DataSetOffset) }, { 3 });
      dimsVar.SetSelection(sel);
      this->DimsBuffer[i * 3 + 0] = nx;
      this->DimsBuffer[i * 3 + 1] = ny;
      this->DimsBuffer[i * 3 + 2] = nz;
      this->Engine.Put<size_t>(dimsVar, &this->DimsBuffer[i * 3]);

      xcOff += nx;
      ycOff += ny;
      zcOff += nz;
    }
  }

  void WriteExplicitCoordinates(const std::vector<PartitionInfo>& partitions)
  {
    size_t cOff = this->CoordOffset;
    for (const auto& p : partitions)
    {
      if (!p.ExplicitCoords.IsValid())
      {
        continue;
      }
      size_t nPts = p.ExplicitCoords.NumValues;
      PutRawArray(this->IO,
                  this->Engine,
                  "coordinates",
                  p.ExplicitCoords,
                  { this->TotalNumberOfCoords, 3 },
                  { cOff, 0 },
                  { nPts, 3 });
      cOff += nPts;
    }
  }

  void WriteStructuredDims(const std::vector<PartitionInfo>& partitions)
  {
    size_t n = partitions.size();
    this->DimsBuffer.resize(n * 3);

    auto dimsVar = this->IO.InquireVariable<size_t>("dims");
    dimsVar.SetShape({ 3 * this->TotalNumberOfDataSets });

    for (size_t i = 0; i < n; i++)
    {
      const auto& p = partitions[i];
      this->DimsBuffer[i * 3 + 0] = p.Dims[0];
      this->DimsBuffer[i * 3 + 1] = p.Dims[1];
      this->DimsBuffer[i * 3 + 2] = p.Dims[2];

      adios2::Box<adios2::Dims> sel({ i * 3 + (3 * this->DataSetOffset) }, { 3 });
      dimsVar.SetSelection(sel);
      this->Engine.Put<size_t>(dimsVar, &this->DimsBuffer[i * 3]);
    }
  }

  void WriteCells(const std::vector<PartitionInfo>& partitions)
  {
    if (this->GlobalCellType == PartitionInfo::CellType::Structured)
    {
      // Nothing to write for structured cells
      return;
    }
    else if (this->GlobalCellType == PartitionInfo::CellType::SingleType)
    {
      this->WriteSingleTypeCells(partitions);
    }
    else if (this->GlobalCellType == PartitionInfo::CellType::PolyData)
    {
      this->WritePolyDataCells(partitions);
    }
    else
    {
      this->WriteExplicitCells(partitions);
    }
  }

  void WriteSingleTypeCells(const std::vector<PartitionInfo>& partitions)
  {
    size_t connOff = this->ConnOffset;
    for (const auto& p : partitions)
    {
      if (!p.SingleTypeConnectivity.IsValid())
      {
        continue;
      }
      size_t n = p.SingleTypeConnectivity.NumValues;
      PutRawArray(this->IO,
                  this->Engine,
                  "connectivity",
                  p.SingleTypeConnectivity,
                  { this->TotalNumberOfConnIds },
                  { connOff },
                  { n });
      connOff += n;
    }
  }

  void WriteExplicitCells(const std::vector<PartitionInfo>& partitions)
  {
    size_t cellOff = this->CellOffset;
    size_t connOff = this->ConnOffset;

    auto shapesVar = this->IO.InquireVariable<uint8_t>("cell_types");
    auto vertsVar = this->IO.InquireVariable<int32_t>("num_verts");

    shapesVar.SetShape({ this->TotalExplicitCells });
    vertsVar.SetShape({ this->TotalExplicitCells });

    for (const auto& p : partitions)
    {
      size_t nCells = p.NumberOfCells();
      if (nCells == 0)
      {
        continue;
      }

      // Cell types
      if (p.ExplicitCellTypes.IsValid())
      {
        adios2::Box<adios2::Dims> sel({ cellOff }, { nCells });
        shapesVar.SetSelection(sel);
        this->Engine.Put<uint8_t>(shapesVar, p.ExplicitCellTypes.GetPointer<uint8_t>());
      }

      // Num verts
      if (p.ExplicitNumVerts.IsValid())
      {
        adios2::Box<adios2::Dims> sel({ cellOff }, { nCells });
        vertsVar.SetSelection(sel);
        this->Engine.Put<int32_t>(vertsVar, p.ExplicitNumVerts.GetPointer<int32_t>());
      }

      // Connectivity
      if (p.ExplicitConnectivity.IsValid())
      {
        size_t nConns = p.ExplicitConnectivity.NumValues;
        PutRawArray(this->IO,
                    this->Engine,
                    "connectivity",
                    p.ExplicitConnectivity,
                    { this->TotalExplicitConns },
                    { connOff },
                    { nConns });
        connOff += nConns;
      }

      cellOff += nCells;
    }
  }

  void WritePolyDataCells(const std::vector<PartitionInfo>& partitions)
  {
    // Per-role write. Each role has its own running offset into the
    // global offsets+connectivity streams; we advance them as we walk
    // partitions on this rank. Skip roles with zero global cells (no
    // variable was defined for them).
    constexpr int kRoles = 4;
    std::array<size_t, kRoles> offsetsCursor;
    std::array<size_t, kRoles> connCursor;
    for (int r = 0; r < kRoles; r++)
    {
      offsetsCursor[r] = this->PolyDataRoles[r].OffsetsRankStart;
      connCursor[r] = this->PolyDataRoles[r].ConnRankStart;
    }

    for (const auto& p : partitions)
    {
      for (int r = 0; r < kRoles; r++)
      {
        if (this->PolyDataRoles[r].TotalOffsets == 0)
        {
          continue;
        }
        auto ref = GetPolyDataRole(p, r);
        if (ref.Offsets.IsValid())
        {
          const size_t n = ref.Offsets.NumValues;
          PutRawArray(this->IO,
                      this->Engine,
                      this->PolyDataRoles[r].Name + "_offsets",
                      ref.Offsets,
                      { this->PolyDataRoles[r].TotalOffsets },
                      { offsetsCursor[r] },
                      { n });
          offsetsCursor[r] += n;
        }
        if (ref.Connectivity.IsValid())
        {
          const size_t n = ref.Connectivity.NumValues;
          PutRawArray(this->IO,
                      this->Engine,
                      this->PolyDataRoles[r].Name + "_connectivity",
                      ref.Connectivity,
                      { this->PolyDataRoles[r].TotalConn },
                      { connCursor[r] },
                      { n });
          connCursor[r] += n;
        }
      }
    }
  }

  void WriteFields(const std::vector<PartitionInfo>& partitions)
  {
    for (const auto& varName : this->PointFieldNames)
    {
      size_t ptsOff = this->DataSetPointsOffset;
      for (const auto& p : partitions)
      {
        size_t nPts = p.NumberOfPoints();
        if (nPts == 0)
        {
          continue;
        }

        for (const auto& f : p.Fields)
        {
          if (f.Name == varName && f.Data.IsValid())
          {
            size_t nComp = static_cast<size_t>(f.Data.NumComponents);
            if (nComp == 1)
            {
              PutRawArray(this->IO,
                          this->Engine,
                          varName,
                          f.Data,
                          { this->TotalNumberOfPoints },
                          { ptsOff },
                          { nPts });
            }
            else
            {
              PutRawArray(this->IO,
                          this->Engine,
                          varName,
                          f.Data,
                          { this->TotalNumberOfPoints, nComp },
                          { ptsOff, 0 },
                          { nPts, nComp });
            }
            break;
          }
        }
        ptsOff += nPts;
      }
    }

    for (const auto& varName : this->CellFieldNames)
    {
      size_t cellsOff = this->DataSetCellsOffset;
      for (const auto& p : partitions)
      {
        size_t nCells = p.NumberOfCells();
        if (nCells == 0)
        {
          continue;
        }

        for (const auto& f : p.Fields)
        {
          if (f.Name == varName && f.Data.IsValid())
          {
            size_t nComp = static_cast<size_t>(f.Data.NumComponents);
            if (nComp == 1)
            {
              PutRawArray(this->IO,
                          this->Engine,
                          varName,
                          f.Data,
                          { this->TotalNumberOfCells },
                          { cellsOff },
                          { nCells });
            }
            else
            {
              PutRawArray(this->IO,
                          this->Engine,
                          varName,
                          f.Data,
                          { this->TotalNumberOfCells, nComp },
                          { cellsOff, 0 },
                          { nCells, nComp });
            }
            break;
          }
        }
        cellsOff += nCells;
      }
    }
  }

  bool ShouldWriteField(const std::string& name) const
  {
    if (this->WriteAll)
    {
      return true;
    }
    return this->FieldsToWrite.find(name) != this->FieldsToWrite.end();
  }

  void DoWrite(const std::vector<PartitionInfo>& partitions)
  {
    if (!this->InStep)
    {
      throw std::logic_error("BeginStep() must be called before Write().");
    }
    if (this->CurrentWriteMode == WriteMode::CellGrid)
    {
      throw std::runtime_error("FidesWriter: cannot mix vtkCellGrid and vtkDataSet writes "
                               "on the same engine.");
    }
    this->CurrentWriteMode = WriteMode::DataSet;

    // Validate each partition before doing any ADIOS operations
    for (const auto& p : partitions)
    {
      ValidatePartitionInfo(p);
    }

    // Validate all local partitions share the same coord/cell types
    if (!partitions.empty())
    {
      auto coordType = partitions[0].Coordinates;
      auto cellType = partitions[0].Cells;
      for (size_t i = 1; i < partitions.size(); ++i)
      {
        if (partitions[i].Coordinates != coordType || partitions[i].Cells != cellType)
        {
          throw std::runtime_error("All partitions must have the same CoordType and CellType. "
                                   "Partition 0 has " +
                                   partitions[0].GetDataModelTypeString() + " but partition " +
                                   std::to_string(i) + " has " +
                                   partitions[i].GetDataModelTypeString() + ".");
        }
      }
    }

    this->ComputeGlobalBlockInfo(partitions);

    // Store local counts for variable definition
    this->LocalPoints = 0;
    this->LocalCells = 0;
    this->LocalConns = 0;
    for (const auto& p : partitions)
    {
      this->LocalPoints += p.NumberOfPoints();
      this->LocalCells += p.NumberOfCells();
      if (this->GlobalCellType == PartitionInfo::CellType::SingleType)
      {
        this->LocalConns += p.SingleTypeConnectivity.NumValues;
      }
      else if (this->GlobalCellType == PartitionInfo::CellType::Explicit)
      {
        this->LocalConns += p.ExplicitConnectivity.NumValues;
      }
    }

    bool firstTime = false;
    if (!this->VariablesDefined)
    {
      this->DefineVariables(partitions);
      this->VariablesDefined = true;
      firstTime = true;
    }

    if (firstTime)
    {
      this->WriteSchema(partitions);
    }

    this->WriteCoordinates(partitions);
    this->WriteCells(partitions);
    this->WriteFields(partitions);

    if (this->TimeSet)
    {
      auto timeVar = this->IO.InquireVariable<double>("time");
      if (!timeVar)
      {
        timeVar = this->IO.DefineVariable<double>("time");
      }
      this->Engine.Put(timeVar, this->CurrentTime);
      this->TimeSet = false;
    }

    this->Engine.PerformPuts();
  }

  // --- CellGrid write path ---

  /// Define ADIOS2 attributes describing the cellgrid structure. Called
  /// once on the first write. The schema and DataModel attributes are
  /// written here too, alongside the structural attributes. \c info
  /// must already have been validated as identical across partitions
  /// for the structural pieces; per-(attr,ct) variable name strings
  /// must come from a non-empty partition.
  void DefineCellGridAttributes(const std::vector<CellGridPartitionInfo>& partitions)
  {
    if (partitions.empty())
    {
      return;
    }
    // First non-empty partition supplies the structure. Multi-partition
    // input must agree, but we don't re-validate here; ValidateMatch is
    // called by DoWriteCellGrid before us.
    const auto& templ = partitions.front();

    // Schema + data model marker. TimeEverSet propagates the existence of
    // any prior SetCurrentTime call -- if the caller intends to use the
    // step axis they MUST call SetCurrentTime before the first Write so
    // the schema, written here, declares step_variable.
    this->IO.DefineAttribute<std::string>("fides/schema",
                                          GenerateCellGridSchemaJSON(templ, this->TimeEverSet));
    this->IO.DefineAttribute<std::string>("Fides_Data_Model", "cell_grid");

    // cell_types: string array of all registered cell type names.
    std::vector<std::string> cellTypeNames;
    cellTypeNames.reserve(templ.CellTypes.size());
    for (const auto& ct : templ.CellTypes)
    {
      cellTypeNames.push_back(ct.CellTypeName);
    }
    if (!cellTypeNames.empty())
    {
      this->IO.DefineAttribute<std::string>(
        "cell_types", cellTypeNames.data(), cellTypeNames.size());
    }

    // <ct>/shape: one string attribute per registered cell type.
    for (const auto& ct : templ.CellTypes)
    {
      this->IO.DefineAttribute<std::string>(ct.CellTypeName + "/shape", ct.ShapeName);
    }

    // Per-attribute structural attributes; per-(attr,ct) DOF metadata
    // and variable-name pointers.
    for (const auto& attr : templ.Attributes)
    {
      this->IO.DefineAttribute<std::string>(attr.Name + "/space", attr.Space);
      this->IO.DefineAttribute<std::int32_t>(attr.Name + "/components", attr.Components);
      this->IO.DefineAttribute<std::int32_t>(attr.Name + "/is_shape", attr.IsShape ? 1 : 0);

      for (const auto& perType : attr.PerCellType)
      {
        const std::string prefix = attr.Name + "/" + perType.CellTypeName;
        this->IO.DefineAttribute<std::string>(prefix + "/function_space", perType.FunctionSpace);
        this->IO.DefineAttribute<std::string>(prefix + "/basis", perType.Basis);
        this->IO.DefineAttribute<std::int32_t>(prefix + "/order", perType.Order);
        this->IO.DefineAttribute<std::string>(prefix + "/dof_sharing", perType.DOFSharing);

        // One <attr>/<ct>/<role> attribute per array role, holding the
        // variable name the reader resolves the role's data through.
        for (const auto& rolePair : perType.Roles)
        {
          const std::string varName =
            CellGridVariableName(rolePair.second.Group, rolePair.second.ArrayName);
          if (!varName.empty())
          {
            this->IO.DefineAttribute<std::string>(prefix + "/" + rolePair.first, varName);
          }
        }

        // vtkDGCell source-spec scalars, emitted only when non-default
        // (matching vtkDGIOResponder); the reader defaults to 0 / false.
        if (perType.Offset != 0)
        {
          this->IO.DefineAttribute<std::int32_t>(prefix + "/offset",
                                                 static_cast<std::int32_t>(perType.Offset));
        }
        if (perType.Blanked)
        {
          this->IO.DefineAttribute<std::int32_t>(prefix + "/blanked", 1);
        }
      }
    }
  }

  /// Define one ADIOS2 local variable per unique (attr, ct, role) tuple
  /// referenced by the cellgrid. Each variable is local (no global
  /// shape) so per-partition Puts each create a new block, which is
  /// what CellGridModel iterates via BlocksInfo on the read side.
  void DefineCellGridVariables(const std::vector<CellGridPartitionInfo>& partitions)
  {
    if (partitions.empty())
    {
      return;
    }
    // Variable types are taken from the first partition that has a
    // non-empty RawArray for the role; later partitions must match.
    std::set<std::string> defined;
    for (const auto& part : partitions)
    {
      for (const auto& attr : part.Attributes)
      {
        for (const auto& perType : attr.PerCellType)
        {
          for (const auto& rolePair : perType.Roles)
          {
            const RawArray& data = rolePair.second.Data;
            const std::string varName =
              CellGridVariableName(rolePair.second.Group, rolePair.second.ArrayName);
            if (varName.empty() || data.NumValues == 0)
            {
              continue;
            }
            if (!defined.insert(varName).second)
            {
              continue;
            }
            DefineLocalVariable(this->IO, varName, data.Type);
          }
        }
      }
    }
  }

  /// Put one ADIOS2 block per partition for each unique (variable name)
  /// referenced by any (attr, ct, role) tuple in that partition.
  /// Variable count is set per-Put via SetSelection so partitions with
  /// different per-block sizes still produce one block each.
  ///
  /// Within a single partition the same connectivity variable is
  /// frequently referenced by many attributes (e.g. shape and every
  /// scalar/vector field share "vtkDGHex/conn"). Without per-partition
  /// dedup each shared variable would get N Puts, producing N stacked
  /// blocks of identical data; the reader's "one block per partition"
  /// assumption (anchored on the values variable) then disagrees with
  /// the connectivity block count.
  void WriteCellGridVariables(const std::vector<CellGridPartitionInfo>& partitions)
  {
    for (const auto& part : partitions)
    {
      std::set<std::string> putThisPartition;
      auto putOnce = [&](const std::string& group, const std::string& name, const RawArray& data) {
        if (data.NumValues == 0)
        {
          return;
        }
        const std::string varName = CellGridVariableName(group, name);
        if (varName.empty())
        {
          return;
        }
        if (!putThisPartition.insert(varName).second)
        {
          return;
        }
        PutLocalRawArray(this->IO, this->Engine, varName, data);
      };
      for (const auto& attr : part.Attributes)
      {
        for (const auto& perType : attr.PerCellType)
        {
          for (const auto& rolePair : perType.Roles)
          {
            putOnce(rolePair.second.Group, rolePair.second.ArrayName, rolePair.second.Data);
          }
        }
      }
    }
  }

  void DoWriteCellGrid(const std::vector<CellGridPartitionInfo>& partitions)
  {
    if (!this->InStep)
    {
      throw std::logic_error("BeginStep() must be called before Write().");
    }
    if (this->CurrentWriteMode == WriteMode::DataSet)
    {
      throw std::runtime_error("FidesWriter: cannot mix vtkDataSet and vtkCellGrid writes "
                               "on the same engine.");
    }
    this->CurrentWriteMode = WriteMode::CellGrid;

    // First write defines the schema, ADIOS2 attributes, and variables.
    // Attributes are file-scope so they only get written once.
    if (!this->VariablesDefined)
    {
      this->DefineCellGridAttributes(partitions);
      this->DefineCellGridVariables(partitions);
      this->VariablesDefined = true;
    }

    this->WriteCellGridVariables(partitions);

    if (this->TimeSet)
    {
      auto timeVar = this->IO.InquireVariable<double>("time");
      if (!timeVar)
      {
        timeVar = this->IO.DefineVariable<double>("time");
      }
      this->Engine.Put(timeVar, this->CurrentTime);
      this->TimeSet = false;
    }

    this->Engine.PerformPuts();
  }

  // --- Member data ---
  std::string OutputFile;
  std::string OutputMode;
  std::string AdiosConfigFile;
  std::map<std::string, std::string> EngineParams;

#if FIDES_USE_MPI
  MPI_Comm Comm = MPI_COMM_NULL;
#endif
  adios2::ADIOS Adios;
  adios2::IO IO;
  adios2::Engine Engine;
  bool AdiosCreated = false;
  bool EngineOpen = false;
  bool VariablesDefined = false;
  bool CloseCalled = false;
  bool InStep = false;

  /// Once one Write overload has been called the engine is locked into
  /// that kind for its lifetime; the schema and variable definitions
  /// would otherwise diverge across steps.
  enum class WriteMode
  {
    Unset,
    DataSet,
    CellGrid
  };
  WriteMode CurrentWriteMode = WriteMode::Unset;

  // Field filtering
  std::set<std::string> FieldsToWrite;
  bool WriteAll = true;

  // Time support
  double CurrentTime = 0.0;
  bool TimeSet = false;
  bool TimeEverSet = false;

  // MPI info
  int Rank = 0;
  int NumRanks = 1;

  // Global block info
  std::vector<size_t> DataSetsPerRank;
  size_t TotalNumberOfDataSets = 0;
  size_t TotalNumberOfPoints = 0;
  size_t TotalNumberOfCells = 0;
  size_t NumberOfDataSets = 0;
  size_t DataSetOffset = 0;
  size_t DataSetPointsOffset = 0;
  size_t DataSetCellsOffset = 0;

  // Local counts (computed per write)
  size_t LocalPoints = 0;
  size_t LocalCells = 0;
  size_t LocalConns = 0;

  // Type info
  PartitionInfo::CoordType GlobalCoordType = PartitionInfo::CoordType::Uniform;
  PartitionInfo::CellType GlobalCellType = PartitionInfo::CellType::Structured;

  // Rectilinear-specific
  size_t NumXCoords = 0, NumYCoords = 0, NumZCoords = 0;
  size_t TotalXCoords = 0, TotalYCoords = 0, TotalZCoords = 0;
  size_t XCoordsOffset = 0, YCoordsOffset = 0, ZCoordsOffset = 0;

  // Single-type unstructured-specific
  size_t TotalNumberOfCoords = 0;
  size_t TotalNumberOfConnIds = 0;
  size_t CoordOffset = 0;
  size_t ConnOffset = 0;
  size_t GlobalVertsPerCell = 0;
  CellShape GlobalCellShape = CellShape::Triangle;

  // Explicit unstructured-specific
  size_t TotalExplicitCells = 0;
  size_t TotalExplicitConns = 0;
  size_t CellOffset = 0;

  // Polydata-specific. One (offsets, connectivity) stream per vtkPolyData
  // cell-array role. Empty roles -- those with zero global cells across
  // all ranks -- get no ADIOS2 variable defined.
  struct PolyDataRoleTotals
  {
    std::string Name;        // "verts", "lines", "polys", "strips"
    size_t LocalOffsets = 0; // this rank's offsets-array contribution
    size_t LocalConn = 0;    // this rank's connectivity-array contribution
    size_t TotalOffsets = 0; // global offsets count summed across ranks
    size_t TotalConn = 0;    // global connectivity count summed across ranks
    size_t OffsetsRankStart = 0;
    size_t ConnRankStart = 0;
    DataType OffsetsType = DataType::Int64;
    DataType ConnType = DataType::Int64;
  };
  std::array<PolyDataRoleTotals, 4> PolyDataRoles = {
    PolyDataRoleTotals{ "verts", 0, 0, 0, 0, 0, 0, DataType::Int64, DataType::Int64 },
    PolyDataRoleTotals{ "lines", 0, 0, 0, 0, 0, 0, DataType::Int64, DataType::Int64 },
    PolyDataRoleTotals{ "polys", 0, 0, 0, 0, 0, 0, DataType::Int64, DataType::Int64 },
    PolyDataRoleTotals{ "strips", 0, 0, 0, 0, 0, 0, DataType::Int64, DataType::Int64 }
  };

  // Buffers for uniform/rectilinear writes
  std::vector<size_t> DimsBuffer;
  std::vector<double> OriginsBuffer;
  std::vector<double> SpacingsBuffer;

  // Field variable names
  std::vector<std::string> PointFieldNames;
  std::vector<std::string> CellFieldNames;
};

// --- FidesWriter public API ---

FidesWriter::FidesWriter(const std::string& outputFile, const std::string& outputMode)
  : PImpl(new Impl(outputFile, outputMode))
{
}

#if FIDES_USE_MPI
FidesWriter::FidesWriter(const std::string& outputFile,
                         MPI_Comm comm,
                         const std::string& outputMode)
  : PImpl(new Impl(outputFile, comm, outputMode))
{
}
#endif

FidesWriter::~FidesWriter()
{
  if (this->PImpl && this->PImpl->EngineOpen && !this->PImpl->CloseCalled)
  {
    if (this->PImpl->InStep)
    {
      this->PImpl->Engine.EndStep();
      this->PImpl->InStep = false;
    }
    this->PImpl->Engine.Close();
    this->PImpl->CloseCalled = true;
  }
}

void FidesWriter::BeginStep()
{
  this->PImpl->EnsureEngineOpen();
  this->PImpl->Engine.BeginStep();
  this->PImpl->InStep = true;
}

void FidesWriter::Write(const std::vector<PartitionInfo>& partitions)
{
  this->PImpl->DoWrite(partitions);
}

void FidesWriter::Write(const std::vector<CellGridPartitionInfo>& partitions)
{
  this->PImpl->DoWriteCellGrid(partitions);
}

void FidesWriter::EndStep()
{
  this->PImpl->Engine.EndStep();
  this->PImpl->InStep = false;
}

void FidesWriter::Close()
{
  if (this->PImpl->InStep)
  {
    throw std::logic_error("EndStep() must be called before Close().");
  }
  if (this->PImpl->EngineOpen && !this->PImpl->CloseCalled)
  {
    this->PImpl->Engine.Close();
    this->PImpl->CloseCalled = true;
  }
}

bool FidesWriter::IsStepOpen() const
{
  return this->PImpl->InStep;
}

void FidesWriter::SetAdiosConfigFile(const std::string& configFile)
{
  if (this->PImpl->EngineOpen)
  {
    throw std::logic_error("SetAdiosConfigFile() must be called before the first BeginStep().");
  }
  this->PImpl->AdiosConfigFile = configFile;
}

void FidesWriter::SetEngineParameters(const std::map<std::string, std::string>& params)
{
  if (this->PImpl->EngineOpen)
  {
    throw std::logic_error("SetEngineParameters() must be called before the first BeginStep().");
  }
  this->PImpl->EngineParams = params;
}

void FidesWriter::SetCurrentTime(double t)
{
  this->PImpl->CurrentTime = t;
  this->PImpl->TimeSet = true;
  this->PImpl->TimeEverSet = true;
}

void FidesWriter::SetWriteFields(const std::set<std::string>& fields)
{
  if (this->PImpl->EngineOpen)
  {
    throw std::logic_error("SetWriteFields() must be called before the first BeginStep().");
  }
  this->PImpl->FieldsToWrite = fields;
  this->PImpl->WriteAll = false;
}

void FidesWriter::SetWriteAllFields(bool writeAll)
{
  if (this->PImpl->EngineOpen)
  {
    throw std::logic_error("SetWriteAllFields() must be called before the first BeginStep().");
  }
  this->PImpl->FieldsToWrite.clear();
  this->PImpl->WriteAll = writeAll;
}

std::string FidesWriter::GenerateSchema(const std::vector<PartitionInfo>& partitions)
{
  if (partitions.empty())
  {
    return "{}";
  }
  std::set<std::string> noFilter;
  return GenerateSchemaJSON(partitions[0], noFilter, partitions.size(), false, true);
}

} // namespace fides
