//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

// Only compiled when FIDES_USE_VTK is enabled (guarded by CMakeLists.txt).

#include <fides/vtk/FidesDataSetWriterVTK.h>

#include <vtkArrayDispatch.h>
#include <vtkCellArray.h>
#include <vtkCellAttribute.h>
#include <vtkCellData.h>
#include <vtkCellGrid.h>
#include <vtkCellTypes.h>
#include <vtkDGCell.h>
#include <vtkDataArray.h>
#include <vtkDataArrayRange.h>
#include <vtkDataSet.h>
#include <vtkImageData.h>
#include <vtkPartitionedDataSet.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkRectilinearGrid.h>
#include <vtkSmartPointer.h>
#include <vtkStringToken.h>
#include <vtkStructuredGrid.h>
#include <vtkUnsignedCharArray.h>
#include <vtkUnstructuredGrid.h>

#include <cstring>
#include <stdexcept>

namespace fides
{

namespace
{

/// Returns true if the given VTK cell type can be written via the
/// SingleType fast path. The 8 linear shapes match fides::CellShape
/// directly; VTK_VOXEL and VTK_PIXEL are accepted via a vertex-order
/// remap on write (see RemapSingleTypeConnectivity below). Everything
/// else (VTK_POLYGON, VTK_TRIANGLE_STRIP, VTK_POLY_VERTEX / POLY_LINE,
/// higher-order shapes) is routed through the Explicit (mixed-type)
/// path instead, which zero-copies the cell-types byte array verbatim
/// and lets VTK construct whatever cells the bytes describe.
bool CanWriteAsSingleType(int vtkType)
{
  switch (vtkType)
  {
    case 1:  // VTK_VERTEX
    case 3:  // VTK_LINE
    case 5:  // VTK_TRIANGLE
    case 8:  // VTK_PIXEL (remapped to VTK_QUAD)
    case 9:  // VTK_QUAD
    case 10: // VTK_TETRA
    case 11: // VTK_VOXEL (remapped to VTK_HEXAHEDRON)
    case 12: // VTK_HEXAHEDRON
    case 13: // VTK_WEDGE
    case 14: // VTK_PYRAMID
      return true;
    default:
      return false;
  }
}

/// Convert VTK cell type to fides CellShape.
/// CellShape enum values match VTK cell type constants directly.
CellShape ConvertVTKCellType(int vtkType)
{
  switch (vtkType)
  {
    case 1:  // VTK_VERTEX
    case 3:  // VTK_LINE
    case 5:  // VTK_TRIANGLE
    case 9:  // VTK_QUAD
    case 10: // VTK_TETRA
    case 12: // VTK_HEXAHEDRON
    case 13: // VTK_WEDGE
    case 14: // VTK_PYRAMID
      return static_cast<CellShape>(vtkType);
    default:
      throw std::runtime_error("ConvertVTKCellType: VTK cell type " + std::to_string(vtkType) +
                               " is not yet supported by Fides");
  }
}

/// One- or two-pair vertex-index swap applied per cell when remapping
/// VTK_VOXEL / VTK_PIXEL connectivity to VTK_HEXAHEDRON / VTK_QUAD.
struct CellVertexRemap
{
  int FirstPairA;
  int FirstPairB;
  int SecondPairA; // -1 if no second pair (e.g. pixel)
  int SecondPairB;
};
// VTK_VOXEL ordering walks ijk monotonically; VTK_HEXAHEDRON walks the
// bottom face CCW then the top face CCW. The two layouts differ by
// swapping vertices 2<->3 on the bottom face and 6<->7 on the top.
constexpr CellVertexRemap RemapVoxelToHex = { 2, 3, 6, 7 };
// VTK_PIXEL -> VTK_QUAD differs by a single swap on the one face.
constexpr CellVertexRemap RemapPixelToQuad = { 2, 3, -1, -1 };

/// Build a new connectivity RawArray with the same element type as
/// \c srcConn, applying \c remap per cell. Used to promote VTK_VOXEL /
/// VTK_PIXEL cells to VTK_HEXAHEDRON / VTK_QUAD so the on-disk file
/// uses a single CCW vertex ordering and reads back as a uniform
/// hex/quad grid through the existing single-type path.
RawArray RemapSingleTypeConnectivity(vtkDataArray* srcConn,
                                     vtkIdType nCells,
                                     int vertsPerCell,
                                     const CellVertexRemap& remap)
{
  RawArray result;
  auto remapTyped = [&](auto* typedConn) {
    using ValueT = typename std::remove_pointer<decltype(typedConn)>::type::ValueType;
    size_t total = static_cast<size_t>(nCells) * static_cast<size_t>(vertsPerCell);
    result = AllocateRawArray<ValueT>(total, 1);
    ValueT* dst = result.GetWritePointer<ValueT>();
    auto src = vtk::DataArrayValueRange<1>(typedConn);
    for (vtkIdType c = 0; c < nCells; c++)
    {
      const size_t base = static_cast<size_t>(c) * static_cast<size_t>(vertsPerCell);
      for (int v = 0; v < vertsPerCell; v++)
      {
        dst[base + v] = static_cast<ValueT>(src[base + v]);
      }
      std::swap(dst[base + remap.FirstPairA], dst[base + remap.FirstPairB]);
      if (remap.SecondPairA >= 0)
      {
        std::swap(dst[base + remap.SecondPairA], dst[base + remap.SecondPairB]);
      }
    }
  };

  using Dispatcher = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Integrals>;
  if (!Dispatcher::Execute(srcConn, remapTyped))
  {
    // Unexpected element type. Fall back to int64 via vtkDataArray's
    // component accessor so the code stays correct even if the dispatch
    // table didn't cover this type.
    size_t total = static_cast<size_t>(nCells) * static_cast<size_t>(vertsPerCell);
    result = AllocateRawArray<int64_t>(total, 1);
    int64_t* dst = result.GetWritePointer<int64_t>();
    for (vtkIdType c = 0; c < nCells; c++)
    {
      const size_t base = static_cast<size_t>(c) * static_cast<size_t>(vertsPerCell);
      for (int v = 0; v < vertsPerCell; v++)
      {
        dst[base + v] = static_cast<int64_t>(srcConn->GetComponent(base + v, 0));
      }
      std::swap(dst[base + remap.FirstPairA], dst[base + remap.FirstPairB]);
      if (remap.SecondPairA >= 0)
      {
        std::swap(dst[base + remap.SecondPairA], dst[base + remap.SecondPairB]);
      }
    }
  }
  return result;
}

/// Wrap a VTK data array into a RawArray (zero-copy; keeps the vtkDataArray alive).
RawArray WrapVTKArray(vtkDataArray* arr)
{
  if (!arr)
  {
    return RawArray();
  }

  int numComp = arr->GetNumberOfComponents();
  vtkIdType numTuples = arr->GetNumberOfTuples();

  DataType dtype;
  int vtkElemType = arr->GetDataType();

  switch (vtkElemType)
  {
    case VTK_FLOAT:
      dtype = DataType::Float32;
      break;
    case VTK_DOUBLE:
      dtype = DataType::Float64;
      break;
    case VTK_INT:
      dtype = DataType::Int32;
      break;
    case VTK_LONG_LONG:
      dtype = DataType::Int64;
      break;
    case VTK_SHORT:
      dtype = DataType::Int16;
      break;
    case VTK_SIGNED_CHAR:
    case VTK_CHAR:
      dtype = DataType::Int8;
      break;
    case VTK_UNSIGNED_CHAR:
      dtype = DataType::UInt8;
      break;
    case VTK_UNSIGNED_SHORT:
      dtype = DataType::UInt16;
      break;
    case VTK_UNSIGNED_INT:
      dtype = DataType::UInt32;
      break;
    case VTK_LONG:
      dtype = (sizeof(long) == 8) ? DataType::Int64 : DataType::Int32;
      break;
    case VTK_UNSIGNED_LONG:
      dtype = (sizeof(long) == 8) ? DataType::UInt64 : DataType::UInt32;
      break;
    case VTK_UNSIGNED_LONG_LONG:
      dtype = DataType::UInt64;
      break;
    case VTK_ID_TYPE:
      // vtkIdType is a platform-dependent signed integer (typically int64_t
      // on 64-bit builds, int32_t when VTK_USE_64BIT_IDS is off). Map to
      // whichever underlying integer it is, since the byte layout is what
      // ADIOS2 actually sees.
      dtype = (sizeof(vtkIdType) == 8) ? DataType::Int64 : DataType::Int32;
      break;
    default:
      throw std::runtime_error("WrapVTKArray: unsupported VTK data type " +
                               std::to_string(vtkElemType));
  }

  // Zero-copy: the shared_ptr custom deleter holds a vtkSmartPointer to keep
  // the VTK array alive as long as the RawArray exists.
  vtkSmartPointer<vtkDataArray> ref = arr;
  std::shared_ptr<void> buf(arr->GetVoidPointer(0), [ref](void*) {});

  return RawArray(std::move(buf), static_cast<size_t>(numTuples), numComp, dtype);
}

/// Extract coordinates from a VTK points array as an explicit RawArray (Nx3 interleaved).
/// Zero-copy: delegates to WrapVTKArray which keeps the underlying vtkDataArray alive.
RawArray ExtractPoints(vtkPoints* points)
{
  if (!points || points->GetNumberOfPoints() == 0)
  {
    return RawArray();
  }

  return WrapVTKArray(points->GetData());
}

/// Returns true if the array name is a VTK ghost array that should be skipped.
/// Full ghost support is deferred to a future release.
bool IsGhostArray(const char* name)
{
  if (!name)
  {
    return false;
  }
  return (std::strcmp(name, "vtkGhostType") == 0 || std::strcmp(name, "vtkGhostCell") == 0);
}

/// Extract fields from VTK point/cell data.
void ExtractFields(vtkDataSet* ds,
                   std::vector<FieldInfo>& fields,
                   const std::set<std::string>& fieldsToWrite)
{
  bool filterFields = !fieldsToWrite.empty();

  // Point data
  vtkPointData* pd = ds->GetPointData();
  if (pd)
  {
    for (int i = 0; i < pd->GetNumberOfArrays(); i++)
    {
      vtkDataArray* arr = pd->GetArray(i);
      if (!arr || !arr->GetName())
      {
        continue;
      }
      if (IsGhostArray(arr->GetName()))
      {
        continue;
      }
      std::string name = arr->GetName();
      if (filterFields && fieldsToWrite.find(name) == fieldsToWrite.end())
      {
        continue;
      }
      FieldInfo fi;
      fi.Name = name;
      fi.Association = FieldAssociation::Points;
      fi.Data = WrapVTKArray(arr);
      fields.push_back(std::move(fi));
    }
  }

  // Cell data
  vtkCellData* cd = ds->GetCellData();
  if (cd)
  {
    for (int i = 0; i < cd->GetNumberOfArrays(); i++)
    {
      vtkDataArray* arr = cd->GetArray(i);
      if (!arr || !arr->GetName())
      {
        continue;
      }
      if (IsGhostArray(arr->GetName()))
      {
        continue;
      }
      std::string name = arr->GetName();
      if (filterFields && fieldsToWrite.find(name) == fieldsToWrite.end())
      {
        continue;
      }
      FieldInfo fi;
      fi.Name = name;
      fi.Association = FieldAssociation::Cells;
      fi.Data = WrapVTKArray(arr);
      fields.push_back(std::move(fi));
    }
  }
}

} // end anon namespace

/// Build a CellGridPartitionInfo::RoleArray for one (role, array) entry.
/// The group is the vtkCellGrid array-group the array belongs to (looked
/// up via the precomputed \c arrayLocations map); the name is the
/// vtkDataArray's own name. Zero-copy via WrapVTKArray. Returns false
/// (leaving \c out untouched) when the entry is not a vtkDataArray.
bool BuildRoleArray(vtkAbstractArray* array,
                    const std::unordered_map<vtkAbstractArray*, vtkStringToken>& arrayLocations,
                    CellGridPartitionInfo::RoleArray& out)
{
  auto* arr = vtkDataArray::SafeDownCast(array);
  if (!arr)
  {
    return false;
  }
  if (arr->GetName())
  {
    out.ArrayName = arr->GetName();
  }
  auto locIt = arrayLocations.find(arr);
  if (locIt != arrayLocations.end())
  {
    out.Group = locIt->second.Data();
  }
  out.Data = WrapVTKArray(arr);
  return true;
}

/// Extract structural metadata (cell types + per-attribute /
/// per-(attr,ct) DOF info), every attribute array role, and the
/// shape attribute's vtkDGCell source spec from a vtkCellGrid into a
/// CellGridPartitionInfo.
CellGridPartitionInfo ExtractCellGridPartition(vtkCellGrid* cg)
{
  CellGridPartitionInfo info;

  // Cell types and their shape names.
  const auto cellTypeNames = cg->GetCellTypes();
  info.CellTypes.reserve(cellTypeNames.size());
  for (const auto& ctName : cellTypeNames)
  {
    CellGridPartitionInfo::CellTypeEntry entry;
    entry.CellTypeName = ctName;
    vtkStringToken ctToken(ctName);
    if (auto* dgCell = vtkDGCell::SafeDownCast(cg->GetCellType(ctToken)))
    {
      entry.ShapeName = vtkDGCell::GetShapeName(dgCell->GetShape()).Data();
    }
    info.CellTypes.push_back(std::move(entry));
  }

  // Precompute array -> group lookup once; every role on every
  // (attribute, cell-type) pair queries it.
  std::unordered_map<vtkAbstractArray*, vtkStringToken> arrayLocations;
  cg->MapArrayLocations(arrayLocations);

  auto attrs = cg->GetCellAttributeList();
  vtkCellAttribute* shapeAttr = cg->GetShapeAttribute();
  info.Attributes.reserve(attrs.size());
  for (const auto& attr : attrs)
  {
    CellGridPartitionInfo::Attribute attrInfo;
    attrInfo.Name = attr->GetName().Data();
    attrInfo.Space = attr->GetSpace().Data();
    attrInfo.Components = attr->GetNumberOfComponents();
    attrInfo.IsShape = (attr.GetPointer() == shapeAttr);

    for (const auto& ctEntry : info.CellTypes)
    {
      vtkStringToken ctToken(ctEntry.CellTypeName);
      auto cellTypeInfo = attr->GetCellTypeInfo(ctToken);
      // Skip cell types this attribute isn't defined on. We use an empty
      // FunctionSpace token as the signal — matches the reader, which
      // gates per-(attr,ct) reads on a non-empty function_space ADIOS2
      // attribute.
      if (!cellTypeInfo.FunctionSpace.IsValid() || cellTypeInfo.FunctionSpace.Data().empty())
      {
        continue;
      }

      CellGridPartitionInfo::AttributePerType perType;
      perType.CellTypeName = ctEntry.CellTypeName;
      perType.FunctionSpace = cellTypeInfo.FunctionSpace.Data();
      perType.Basis = cellTypeInfo.Basis.IsValid() ? cellTypeInfo.Basis.Data() : std::string();
      perType.Order = cellTypeInfo.Order;
      if (cellTypeInfo.DOFSharing.IsValid())
      {
        perType.DOFSharing = cellTypeInfo.DOFSharing.Data();
      }

      // Every attribute array role (values, connectivity, ...). Role
      // tokens always carry their string data here since they were
      // created from role-name strings.
      for (const auto& rolePair : cellTypeInfo.ArraysByRole)
      {
        if (!rolePair.first.HasData())
        {
          continue;
        }
        CellGridPartitionInfo::RoleArray roleArray;
        if (BuildRoleArray(rolePair.second, arrayLocations, roleArray))
        {
          perType.Roles[rolePair.first.Data()] = std::move(roleArray);
        }
      }

      // The shape attribute also owns the vtkDGCell source spec, which
      // lives outside ArraysByRole: NodalGhostMarks (emitted as the
      // "ghost-node" role), Offset, and Blanked.
      if (attrInfo.IsShape)
      {
        if (auto* dgCell = vtkDGCell::SafeDownCast(cg->GetCellType(ctToken)))
        {
          const auto& spec = dgCell->GetCellSpec();
          perType.Offset = static_cast<int64_t>(spec.Offset);
          perType.Blanked = spec.Blanked;
          if (spec.NodalGhostMarks)
          {
            CellGridPartitionInfo::RoleArray ghost;
            if (BuildRoleArray(spec.NodalGhostMarks, arrayLocations, ghost))
            {
              if (ghost.ArrayName.empty())
              {
                ghost.ArrayName = "ghost-node";
              }
              perType.Roles["ghost-node"] = std::move(ghost);
            }
          }
        }
      }

      attrInfo.PerCellType.push_back(std::move(perType));
    }
    info.Attributes.push_back(std::move(attrInfo));
  }

  return info;
}

VTKExtraction ExtractVTKPartitions(vtkPartitionedDataSet* dataSets,
                                   const std::set<std::string>& fieldsToWrite)
{
  VTKExtraction result;
  if (!dataSets)
  {
    return result;
  }

  unsigned int nParts = dataSets->GetNumberOfPartitions();
  for (unsigned int i = 0; i < nParts; i++)
  {
    vtkDataObject* obj = dataSets->GetPartitionAsDataObject(i);
    if (!obj)
    {
      continue;
    }

    if (auto* cg = vtkCellGrid::SafeDownCast(obj))
    {
      result.CellGrids.push_back(ExtractCellGridPartition(cg));
      continue;
    }

    auto* ds = vtkDataSet::SafeDownCast(obj);
    if (!ds || ds->GetNumberOfPoints() == 0)
    {
      continue;
    }

    PartitionInfo pi;

    if (auto* imageData = vtkImageData::SafeDownCast(ds))
    {
      pi.Coordinates = PartitionInfo::CoordType::Uniform;
      pi.Cells = PartitionInfo::CellType::Structured;

      int dims[3];
      imageData->GetDimensions(dims);
      double origin[3];
      imageData->GetOrigin(origin);
      double spacing[3];
      imageData->GetSpacing(spacing);

      for (int j = 0; j < 3; j++)
      {
        pi.Dims[j] = static_cast<size_t>(dims[j]);
        pi.Origin[j] = origin[j];
        pi.Spacing[j] = spacing[j];
      }
    }
    else if (auto* rectGrid = vtkRectilinearGrid::SafeDownCast(ds))
    {
      pi.Coordinates = PartitionInfo::CoordType::Rectilinear;
      pi.Cells = PartitionInfo::CellType::Structured;

      pi.XCoords = WrapVTKArray(rectGrid->GetXCoordinates());
      pi.YCoords = WrapVTKArray(rectGrid->GetYCoordinates());
      pi.ZCoords = WrapVTKArray(rectGrid->GetZCoordinates());
    }
    else if (auto* structGrid = vtkStructuredGrid::SafeDownCast(ds))
    {
      pi.Coordinates = PartitionInfo::CoordType::Explicit;
      pi.Cells = PartitionInfo::CellType::Structured;

      int dims[3];
      structGrid->GetDimensions(dims);
      for (int j = 0; j < 3; j++)
      {
        pi.Dims[j] = static_cast<size_t>(dims[j]);
      }

      pi.ExplicitCoords = ExtractPoints(structGrid->GetPoints());
    }
    else if (auto* ugrid = vtkUnstructuredGrid::SafeDownCast(ds))
    {
      pi.Coordinates = PartitionInfo::CoordType::Explicit;
      pi.ExplicitCoords = ExtractPoints(ugrid->GetPoints());

      vtkIdType nCells = ugrid->GetNumberOfCells();
      // Use the SingleType fast path only when the grid is homogeneous
      // AND its cell type is one that the SingleType schema can express.
      // Homogeneous-but-unsupported (e.g. all VTK_POLYGON or all
      // VTK_TRIANGLE_STRIP) falls through to the Explicit path; the
      // Explicit path zero-copies VTK's type byte array, so any cell
      // type VTK itself can construct round-trips cleanly.
      const bool homogeneousFast =
        nCells > 0 && ugrid->IsHomogeneous() && CanWriteAsSingleType(ugrid->GetCellType(0));
      if (nCells == 0)
      {
        pi.Cells = PartitionInfo::CellType::SingleType;
      }
      else if (homogeneousFast)
      {
        // All cells are the same type and SingleType-supported.
        // (Higher-order homogeneous shapes still take the Explicit fallback
        // until fides::CellShape gains quadratic / Lagrange / Bezier
        // entries -- correct, just not as compact on disk.)
        pi.Cells = PartitionInfo::CellType::SingleType;
        int firstType = ugrid->GetCellType(0);
        pi.VertsPerCell = ugrid->GetCell(0)->GetNumberOfPoints();
        vtkCellArray* cells = ugrid->GetCells();

        // Voxel and pixel are topologically identical to hex and quad but
        // use VTK's ijk-sorted vertex ordering rather than the CCW ordering
        // fides expects. Promote them here so the on-disk file uses a
        // single ordering and reads back through the existing single-type
        // hex/quad path; this is the same convention vtkUnstructuredGrid
        // uses when it has to materialize a hex/quad surface from voxel
        // input. Loses zero-copy on connectivity for these two types.
        if (firstType == 11) // VTK_VOXEL
        {
          pi.SingleCellShape = CellShape::Hexahedron;
          pi.SingleTypeConnectivity = RemapSingleTypeConnectivity(
            cells->GetConnectivityArray(), nCells, pi.VertsPerCell, RemapVoxelToHex);
        }
        else if (firstType == 8) // VTK_PIXEL
        {
          pi.SingleCellShape = CellShape::Quad;
          pi.SingleTypeConnectivity = RemapSingleTypeConnectivity(
            cells->GetConnectivityArray(), nCells, pi.VertsPerCell, RemapPixelToQuad);
        }
        else
        {
          pi.SingleCellShape = ConvertVTKCellType(firstType);
          // Zero-copy connectivity from the VTK cell array
          pi.SingleTypeConnectivity = WrapVTKArray(cells->GetConnectivityArray());
        }
      }
      else
      {
        // Mixed cell types
        pi.Cells = PartitionInfo::CellType::Explicit;

        vtkCellArray* cells = ugrid->GetCells();

        // Cell types: zero-copy since CellShape enum values match VTK constants.
        pi.ExplicitCellTypes = WrapVTKArray(ugrid->GetCellTypes());

        // Num verts: derived from VTK's offsets array (offsets[c+1] - offsets[c]).
        // TODO: The fides schema currently stores per-cell vertex counts, but
        // both VTK and Viskores natively store offsets (cumulative indices into
        // the connectivity array). This forces a conversion here on write, and
        // the inverse conversion on read for both backends (VTKBuilder does a
        // prefix sum, and the Viskores reader calls ScanExtended). If the schema
        // were extended to also support an "offsets" array, this loop and the
        // corresponding ones on the read side could be eliminated, allowing a
        // zero-copy of cells->GetOffsetsArray() instead.
        vtkDataArray* offsetsArr = cells->GetOffsetsArray();
        auto nVertsRaw = AllocateRawArray<int32_t>(static_cast<size_t>(nCells), 1);
        int32_t* nVertsBuf = nVertsRaw.GetWritePointer<int32_t>();

        auto computeNumVerts = [&](auto* typedOffsets) {
          auto offsets = vtk::DataArrayValueRange<1>(typedOffsets);
          for (vtkIdType c = 0; c < nCells; c++)
          {
            nVertsBuf[c] = static_cast<int32_t>(offsets[c + 1] - offsets[c]);
          }
        };

        using Dispatcher = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Integrals>;
        if (!Dispatcher::Execute(offsetsArr, computeNumVerts))
        {
          // Fallback for unexpected types
          for (vtkIdType c = 0; c < nCells; c++)
          {
            nVertsBuf[c] = static_cast<int32_t>(offsetsArr->GetComponent(c + 1, 0) -
                                                offsetsArr->GetComponent(c, 0));
          }
        }
        pi.ExplicitNumVerts = nVertsRaw;

        // Connectivity: zero-copy from VTK cell array.
        pi.ExplicitConnectivity = WrapVTKArray(cells->GetConnectivityArray());
      }
    }
    else if (auto* pd = vtkPolyData::SafeDownCast(ds))
    {
      pi.Coordinates = PartitionInfo::CoordType::Explicit;
      pi.ExplicitCoords = ExtractPoints(pd->GetPoints());
      pi.Cells = PartitionInfo::CellType::PolyData;

      // Mirror vtkPolyData's four cell arrays directly onto the
      // PartitionInfo (offsets, connectivity) pairs. Any role with no
      // cells stays as a default-constructed (empty) RawArray pair; the
      // writer and reader skip it. Zero-copy throughout: no merging, no
      // strip expansion, no synthetic per-cell type bytes -- the cell
      // type semantics are implicit per role + per-cell vertex count,
      // exactly how vtkCellArray already stores them.
      auto wrapRole = [](vtkCellArray* ca, RawArray& offsetsOut, RawArray& connOut) {
        if (!ca || ca->GetNumberOfCells() == 0)
        {
          return;
        }
        offsetsOut = WrapVTKArray(ca->GetOffsetsArray());
        connOut = WrapVTKArray(ca->GetConnectivityArray());
      };
      wrapRole(pd->GetVerts(), pi.PolyDataVertsOffsets, pi.PolyDataVertsConnectivity);
      wrapRole(pd->GetLines(), pi.PolyDataLinesOffsets, pi.PolyDataLinesConnectivity);
      wrapRole(pd->GetPolys(), pi.PolyDataPolysOffsets, pi.PolyDataPolysConnectivity);
      wrapRole(pd->GetStrips(), pi.PolyDataStripsOffsets, pi.PolyDataStripsConnectivity);
    }
    else
    {
      throw std::runtime_error("FidesDataSetWriter: unsupported VTK dataset type");
    }

    // Extract fields
    ExtractFields(ds, pi.Fields, fieldsToWrite);
    result.DataSets.push_back(std::move(pi));
  }

  // Mixed-kind input is rejected: the writer's bookkeeping (schema,
  // partition offsets, ADIOS2 variable layout) is per-kind, and the
  // upstream pipeline doesn't currently emit a mixed step. If support
  // is needed later, this is the place to lift the restriction.
  if (!result.DataSets.empty() && !result.CellGrids.empty())
  {
    throw std::runtime_error("FidesDataSetWriter: input vtkPartitionedDataSet mixes vtkDataSet and "
                             "vtkCellGrid partitions; this is not supported.");
  }

  return result;
}

} // namespace fides
