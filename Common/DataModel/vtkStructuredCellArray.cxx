// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkStructuredCellArray.h"

#include "vtkIdList.h"
#include "vtkImplicitArray.h"
#include "vtkObjectFactory.h"
#include "vtkStructuredData.h"

#include <array>

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
struct vtkStructuredCellArray::vtkStructuredCellBackend
{
  //------------------------------------------------------------------------------
  vtkStructuredCellBackend() = default;

  //------------------------------------------------------------------------------
  virtual ~vtkStructuredCellBackend() = default;

  //------------------------------------------------------------------------------
  virtual int GetCellSize() const = 0;

  //------------------------------------------------------------------------------
  virtual void mapStructuredTuple(int ijk[3], vtkIdType* values) const = 0;

  //------------------------------------------------------------------------------
  virtual void mapTuple(vtkIdType tupleId, vtkIdType* values) const = 0;

  //------------------------------------------------------------------------------
  virtual vtkIdType mapComponent(vtkIdType tupleId, int comp) const = 0;

  //------------------------------------------------------------------------------
  virtual vtkIdType map(vtkIdType valueId) const = 0;
};

//------------------------------------------------------------------------------
namespace vtkShiftLUT
{
constexpr std::array<int, 8> ShiftLUT0 = { { 0, 0, 0, 0, 0, 0, 0, 0 } };
constexpr std::array<int, 8> ShiftLUT3 = { { 0, 0, 0, 0, 1, 1, 1, 1 } };
// use for voxels/pixels
constexpr std::array<int, 8> ShiftLUT1 = { { 0, 1, 0, 1, 0, 1, 0, 1 } };
constexpr std::array<int, 8> ShiftLUT2 = { { 0, 0, 1, 1, 0, 0, 1, 1 } };
// used for hexes/quads
constexpr std::array<int, 8> ShiftLUTx = { { 0, 1, 1, 0, 0, 1, 1, 0 } };
constexpr std::array<int, 8> ShiftLUTy = { { 0, 0, 1, 1, 0, 0, 1, 1 } };

//------------------------------------------------------------------------------
template <int DataDescription, bool UsePixelVoxelOrientation>
constexpr std::array<std::array<int, 8>, 3> GetShiftLUT();

//------------------------------------------------------------------------------
template <>
constexpr std::array<std::array<int, 8>, 3> GetShiftLUT<VTK_EMPTY, true>()
{
  return { { ShiftLUT0, ShiftLUT0, ShiftLUT0 } };
}
//------------------------------------------------------------------------------
template <>
constexpr std::array<std::array<int, 8>, 3> GetShiftLUT<VTK_SINGLE_POINT, true>()
{
  return { { ShiftLUT0, ShiftLUT0, ShiftLUT0 } };
}
//------------------------------------------------------------------------------
template <>
constexpr std::array<std::array<int, 8>, 3> GetShiftLUT<VTK_X_LINE, true>()
{
  return { { ShiftLUT1, ShiftLUT0, ShiftLUT0 } };
}
//------------------------------------------------------------------------------
template <>
constexpr std::array<std::array<int, 8>, 3> GetShiftLUT<VTK_Y_LINE, true>()
{
  return { { ShiftLUT0, ShiftLUT1, ShiftLUT0 } };
}
//------------------------------------------------------------------------------
template <>
constexpr std::array<std::array<int, 8>, 3> GetShiftLUT<VTK_Z_LINE, true>()
{
  return { { ShiftLUT0, ShiftLUT0, ShiftLUT1 } };
}
//------------------------------------------------------------------------------
template <>
constexpr std::array<std::array<int, 8>, 3> GetShiftLUT<VTK_XY_PLANE, true>()
{
  return { { ShiftLUT1, ShiftLUT2, ShiftLUT0 } };
}
//------------------------------------------------------------------------------
template <>
constexpr std::array<std::array<int, 8>, 3> GetShiftLUT<VTK_YZ_PLANE, true>()
{
  return { { ShiftLUT0, ShiftLUT1, ShiftLUT2 } };
}
//------------------------------------------------------------------------------
template <>
constexpr std::array<std::array<int, 8>, 3> GetShiftLUT<VTK_XZ_PLANE, true>()
{
  return { { ShiftLUT1, ShiftLUT0, ShiftLUT2 } };
}
//------------------------------------------------------------------------------
template <>
constexpr std::array<std::array<int, 8>, 3> GetShiftLUT<VTK_XYZ_GRID, true>()
{
  return { { ShiftLUT1, ShiftLUT2, ShiftLUT3 } };
}
//------------------------------------------------------------------------------
template <>
constexpr std::array<std::array<int, 8>, 3> GetShiftLUT<VTK_EMPTY, false>()
{
  return { { ShiftLUT0, ShiftLUT0, ShiftLUT0 } };
}
//------------------------------------------------------------------------------
template <>
constexpr std::array<std::array<int, 8>, 3> GetShiftLUT<VTK_SINGLE_POINT, false>()
{
  return { { ShiftLUT0, ShiftLUT0, ShiftLUT0 } };
}
//------------------------------------------------------------------------------
template <>
constexpr std::array<std::array<int, 8>, 3> GetShiftLUT<VTK_X_LINE, false>()
{
  return { { ShiftLUTx, ShiftLUT0, ShiftLUT0 } };
}
//------------------------------------------------------------------------------
template <>
constexpr std::array<std::array<int, 8>, 3> GetShiftLUT<VTK_Y_LINE, false>()
{
  return { { ShiftLUT0, ShiftLUTx, ShiftLUT0 } };
}
//------------------------------------------------------------------------------
template <>
constexpr std::array<std::array<int, 8>, 3> GetShiftLUT<VTK_Z_LINE, false>()
{
  return { { ShiftLUT0, ShiftLUT0, ShiftLUTx } };
}
//------------------------------------------------------------------------------
template <>
constexpr std::array<std::array<int, 8>, 3> GetShiftLUT<VTK_XY_PLANE, false>()
{
  return { { ShiftLUTx, ShiftLUTy, ShiftLUT0 } };
}
//------------------------------------------------------------------------------
template <>
constexpr std::array<std::array<int, 8>, 3> GetShiftLUT<VTK_YZ_PLANE, false>()
{
  return { { ShiftLUT0, ShiftLUTx, ShiftLUTy } };
}
//------------------------------------------------------------------------------
template <>
constexpr std::array<std::array<int, 8>, 3> GetShiftLUT<VTK_XZ_PLANE, false>()
{
  return { { ShiftLUTx, ShiftLUT0, ShiftLUTy } };
}
//------------------------------------------------------------------------------
template <>
constexpr std::array<std::array<int, 8>, 3> GetShiftLUT<VTK_XYZ_GRID, false>()
{
  return { { ShiftLUTx, ShiftLUTy, ShiftLUT3 } };
}
}

//------------------------------------------------------------------------------
template <int DataDescription, bool UsePixelVoxelOrientation>
struct vtkStructuredCellArray::vtkStructuredTCellBackend : public vtkStructuredCellBackend
{
  // static constexpr members
  static constexpr int CellSize = DataDescription == VTK_XYZ_GRID
    ? 8
    : DataDescription == VTK_XY_PLANE || DataDescription == VTK_YZ_PLANE ||
        DataDescription == VTK_XZ_PLANE
      ? 4
      : DataDescription == VTK_X_LINE || DataDescription == VTK_Y_LINE ||
          DataDescription == VTK_Z_LINE
        ? 2
        : DataDescription == VTK_SINGLE_POINT ? 1 : 0;

  static constexpr vtkIdType ValidCellSize =
    vtkStructuredTCellBackend::CellSize > 0 ? vtkStructuredTCellBackend::CellSize : 1;

  static constexpr std::array<std::array<int, 8>, 3> ShiftLUT =
    vtkShiftLUT::GetShiftLUT<DataDescription, UsePixelVoxelOrientation>();

  // members
  std::array<vtkIdType, 3> CellDimensions;
  const int PYStride;
  const int PZStride;

  //------------------------------------------------------------------------------
  vtkStructuredTCellBackend() = default;

  //------------------------------------------------------------------------------
  vtkStructuredTCellBackend(int dims[3])
    : CellDimensions({ { dims[0] - 1, dims[1] - 1, dims[2] - 1 } })
    , PYStride(dims[0])
    , PZStride(dims[0] * dims[1])
  {
  }

  //------------------------------------------------------------------------------
  ~vtkStructuredTCellBackend() override = default;

  //------------------------------------------------------------------------------
  int GetCellSize() const override { return CellSize; }

  //------------------------------------------------------------------------------
  template <int Description = DataDescription>
  typename std::enable_if<(Description == VTK_EMPTY), void>::type VTK_ALWAYS_INLINE
  ComputeCellStructuredCoords(vtkIdType vtkNotUsed(cellId), int* ijk) const
  {
    ijk[0] = ijk[1] = ijk[2] = 0;
  }

  //------------------------------------------------------------------------------
  template <int Description = DataDescription>
  typename std::enable_if<(Description == VTK_SINGLE_POINT), void>::type VTK_ALWAYS_INLINE
  ComputeCellStructuredCoords(vtkIdType vtkNotUsed(cellId), int* ijk) const
  {
    ijk[0] = ijk[1] = ijk[2] = 0;
  }

  //------------------------------------------------------------------------------
  template <int Description = DataDescription>
  typename std::enable_if<(Description == VTK_X_LINE), void>::type VTK_ALWAYS_INLINE
  ComputeCellStructuredCoords(vtkIdType cellId, int* ijk) const
  {
    ijk[0] = cellId;
    ijk[1] = 0;
    ijk[2] = 0;
  }

  //------------------------------------------------------------------------------
  template <int Description = DataDescription>
  typename std::enable_if<(Description == VTK_Y_LINE), void>::type VTK_ALWAYS_INLINE
  ComputeCellStructuredCoords(vtkIdType cellId, int* ijk) const
  {
    ijk[0] = 0;
    ijk[1] = cellId;
    ijk[2] = 0;
  }

  //------------------------------------------------------------------------------
  template <int Description = DataDescription>
  typename std::enable_if<(Description == VTK_Z_LINE), void>::type VTK_ALWAYS_INLINE
  ComputeCellStructuredCoords(vtkIdType cellId, int* ijk) const
  {
    ijk[0] = 0;
    ijk[1] = 0;
    ijk[2] = cellId;
  }

  //------------------------------------------------------------------------------
  template <int Description = DataDescription>
  typename std::enable_if<(Description == VTK_XY_PLANE), void>::type VTK_ALWAYS_INLINE
  ComputeCellStructuredCoords(vtkIdType cellId, int* ijk) const
  {
    const auto div = std::div(cellId, this->CellDimensions[0]);
    ijk[0] = div.rem;
    ijk[1] = div.quot;
    ijk[2] = 0;
  }

  //------------------------------------------------------------------------------
  template <int Description = DataDescription>
  typename std::enable_if<(Description == VTK_YZ_PLANE), void>::type VTK_ALWAYS_INLINE
  ComputeCellStructuredCoords(vtkIdType cellId, int* ijk) const
  {
    const auto div = std::div(cellId, this->CellDimensions[1]);
    ijk[0] = 0;
    ijk[1] = div.rem;
    ijk[2] = div.quot;
  }

  //------------------------------------------------------------------------------
  template <int Description = DataDescription>
  typename std::enable_if<(Description == VTK_XZ_PLANE), void>::type VTK_ALWAYS_INLINE
  ComputeCellStructuredCoords(vtkIdType cellId, int* ijk) const
  {
    const auto div = std::div(cellId, this->CellDimensions[0]);
    ijk[0] = div.rem;
    ijk[1] = 0;
    ijk[2] = div.quot;
  }

  //------------------------------------------------------------------------------
  template <int Description = DataDescription>
  typename std::enable_if<(Description == VTK_XYZ_GRID), void>::type VTK_ALWAYS_INLINE
  ComputeCellStructuredCoords(vtkIdType cellId, int* ijk) const
  {
    const auto div1 = std::div(cellId, this->CellDimensions[0]);
    const auto div2 = std::div(div1.quot, this->CellDimensions[1]);
    ijk[0] = div1.rem;
    ijk[1] = div2.rem;
    ijk[2] = div2.quot;
  }

  //------------------------------------------------------------------------------
  void mapStructuredTuple(int ijk[3], vtkIdType* values) const override
  {
    for (int comp = 0; comp < vtkStructuredTCellBackend::CellSize; ++comp)
    {
      values[comp] =
        (static_cast<vtkIdType>(ijk[0]) + vtkStructuredTCellBackend::ShiftLUT[0][comp]) +
        (static_cast<vtkIdType>(ijk[1]) + vtkStructuredTCellBackend::ShiftLUT[1][comp]) *
          this->PYStride +
        (static_cast<vtkIdType>(ijk[2]) + vtkStructuredTCellBackend::ShiftLUT[2][comp]) *
          this->PZStride;
    }
  }

  //------------------------------------------------------------------------------
  void mapTuple(vtkIdType tupleId, vtkIdType* values) const override
  {
    int ijk[3];
    this->ComputeCellStructuredCoords(tupleId, ijk);
    for (int comp = 0; comp < vtkStructuredTCellBackend::CellSize; ++comp)
    {
      values[comp] =
        (static_cast<vtkIdType>(ijk[0]) + vtkStructuredTCellBackend::ShiftLUT[0][comp]) +
        (static_cast<vtkIdType>(ijk[1]) + vtkStructuredTCellBackend::ShiftLUT[1][comp]) *
          this->PYStride +
        (static_cast<vtkIdType>(ijk[2]) + vtkStructuredTCellBackend::ShiftLUT[2][comp]) *
          this->PZStride;
    }
  }

  //------------------------------------------------------------------------------
  vtkIdType mapComponent(vtkIdType tupleId, int comp) const override
  {
    int ijk[3];
    this->ComputeCellStructuredCoords(tupleId, ijk);
    return (ijk[0] + vtkStructuredTCellBackend::ShiftLUT[0][comp]) +
      (ijk[1] + vtkStructuredTCellBackend::ShiftLUT[1][comp]) * this->PYStride +
      (ijk[2] + vtkStructuredTCellBackend::ShiftLUT[2][comp]) * this->PZStride;
  }

  //------------------------------------------------------------------------------
  vtkIdType map(vtkIdType valueId) const override
  {
    const auto div = std::div(valueId, vtkStructuredTCellBackend::ValidCellSize);
    return this->mapComponent(div.quot, div.rem);
  }
};

//------------------------------------------------------------------------------
template <int DataDescription, bool UsePixelVoxelOrientation>
constexpr std::array<std::array<int, 8>, 3> vtkStructuredCellArray::vtkStructuredTCellBackend<
  DataDescription, UsePixelVoxelOrientation>::ShiftLUT;

//------------------------------------------------------------------------------
// template instantiated for each data description
template struct vtkStructuredCellArray::vtkStructuredTCellBackend<VTK_SINGLE_POINT, true>;
template struct vtkStructuredCellArray::vtkStructuredTCellBackend<VTK_X_LINE, true>;
template struct vtkStructuredCellArray::vtkStructuredTCellBackend<VTK_Y_LINE, true>;
template struct vtkStructuredCellArray::vtkStructuredTCellBackend<VTK_Z_LINE, true>;
template struct vtkStructuredCellArray::vtkStructuredTCellBackend<VTK_XY_PLANE, true>;
template struct vtkStructuredCellArray::vtkStructuredTCellBackend<VTK_YZ_PLANE, true>;
template struct vtkStructuredCellArray::vtkStructuredTCellBackend<VTK_XZ_PLANE, true>;
template struct vtkStructuredCellArray::vtkStructuredTCellBackend<VTK_XYZ_GRID, true>;
template struct vtkStructuredCellArray::vtkStructuredTCellBackend<VTK_EMPTY, false>;
template struct vtkStructuredCellArray::vtkStructuredTCellBackend<VTK_SINGLE_POINT, false>;
template struct vtkStructuredCellArray::vtkStructuredTCellBackend<VTK_X_LINE, false>;
template struct vtkStructuredCellArray::vtkStructuredTCellBackend<VTK_Y_LINE, false>;
template struct vtkStructuredCellArray::vtkStructuredTCellBackend<VTK_Z_LINE, false>;
template struct vtkStructuredCellArray::vtkStructuredTCellBackend<VTK_XY_PLANE, false>;
template struct vtkStructuredCellArray::vtkStructuredTCellBackend<VTK_YZ_PLANE, false>;
template struct vtkStructuredCellArray::vtkStructuredTCellBackend<VTK_XZ_PLANE, false>;
template struct vtkStructuredCellArray::vtkStructuredTCellBackend<VTK_XYZ_GRID, false>;

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkStructuredCellArray);

//------------------------------------------------------------------------------
vtkStructuredCellArray::vtkStructuredCellArray() = default;

//------------------------------------------------------------------------------
vtkStructuredCellArray::~vtkStructuredCellArray() = default;

//------------------------------------------------------------------------------
void vtkStructuredCellArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->Connectivity)
  {
    this->Connectivity->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Connectivity: (nullptr)\n";
  }
}

//------------------------------------------------------------------------------
void vtkStructuredCellArray::Initialize()
{
  this->Connectivity->Initialize();
}

//------------------------------------------------------------------------------
vtkIdType vtkStructuredCellArray::GetNumberOfCells() const
{
  return this->Connectivity->GetNumberOfTuples();
}

//------------------------------------------------------------------------------
vtkIdType vtkStructuredCellArray::GetNumberOfOffsets() const
{
  return this->Connectivity->GetNumberOfTuples() + 1;
}

//------------------------------------------------------------------------------
vtkIdType vtkStructuredCellArray::GetOffset(vtkIdType cellId)
{
  return this->Connectivity->GetNumberOfComponents() * cellId;
}

//------------------------------------------------------------------------------
vtkIdType vtkStructuredCellArray::GetNumberOfConnectivityIds() const
{
  return this->Connectivity->GetNumberOfTuples() * this->Connectivity->GetNumberOfComponents();
}

//------------------------------------------------------------------------------
void vtkStructuredCellArray::SetData(int extent[6], bool usePixelVoxelOrientation)
{
  this->Connectivity = vtkSmartPointer<vtkImplicitArray<vtkStructuredCellBackend>>::New();
  int dims[3];
  vtkStructuredData::GetDimensionsFromExtent(extent, dims);
  const auto description = vtkStructuredData::GetDataDescription(dims);
  std::shared_ptr<vtkStructuredCellBackend> backEnd;
  if (usePixelVoxelOrientation)
  {
    switch (description)
    {
      case VTK_EMPTY:
      {
        using TBackend = vtkStructuredTCellBackend<VTK_EMPTY, true>;
        backEnd = std::make_shared<TBackend>(dims);
        break;
      }
      case VTK_SINGLE_POINT:
      {
        using TBackend = vtkStructuredTCellBackend<VTK_SINGLE_POINT, true>;
        backEnd = std::make_shared<TBackend>(dims);
        break;
      }
      case VTK_X_LINE:
      {
        using TBackend = vtkStructuredTCellBackend<VTK_X_LINE, true>;
        backEnd = std::make_shared<TBackend>(dims);
        break;
      }
      case VTK_Y_LINE:
      {
        using TBackend = vtkStructuredTCellBackend<VTK_Y_LINE, true>;
        backEnd = std::make_shared<TBackend>(dims);
        break;
      }
      case VTK_Z_LINE:
      {
        using TBackend = vtkStructuredTCellBackend<VTK_Z_LINE, true>;
        backEnd = std::make_shared<TBackend>(dims);
        break;
      }
      case VTK_XY_PLANE:
      {
        using TBackend = vtkStructuredTCellBackend<VTK_XY_PLANE, true>;
        backEnd = std::make_shared<TBackend>(dims);
        break;
      }
      case VTK_YZ_PLANE:
      {
        using TBackend = vtkStructuredTCellBackend<VTK_YZ_PLANE, true>;
        backEnd = std::make_shared<TBackend>(dims);
        break;
      }
      case VTK_XZ_PLANE:
      {
        using TBackend = vtkStructuredTCellBackend<VTK_XZ_PLANE, true>;
        backEnd = std::make_shared<TBackend>(dims);
        break;
      }
      case VTK_XYZ_GRID:
      {
        using TBackend = vtkStructuredTCellBackend<VTK_XYZ_GRID, true>;
        backEnd = std::make_shared<TBackend>(dims);
        break;
      }
      default:
      {
        vtkErrorMacro("Unsupported data description: " << description);
        return;
      }
    }
  }
  else
  {
    switch (description)
    {
      case VTK_EMPTY:
      {
        using TBackend = vtkStructuredTCellBackend<VTK_EMPTY, false>;
        backEnd = std::make_shared<TBackend>(dims);
        break;
      }
      case VTK_SINGLE_POINT:
      {
        using TBackend = vtkStructuredTCellBackend<VTK_SINGLE_POINT, false>;
        backEnd = std::make_shared<TBackend>(dims);
        break;
      }
      case VTK_X_LINE:
      {
        using TBackend = vtkStructuredTCellBackend<VTK_X_LINE, false>;
        backEnd = std::make_shared<TBackend>(dims);
        break;
      }
      case VTK_Y_LINE:
      {
        using TBackend = vtkStructuredTCellBackend<VTK_Y_LINE, false>;
        backEnd = std::make_shared<TBackend>(dims);
        break;
      }
      case VTK_Z_LINE:
      {
        using TBackend = vtkStructuredTCellBackend<VTK_Z_LINE, false>;
        backEnd = std::make_shared<TBackend>(dims);
        break;
      }
      case VTK_XY_PLANE:
      {
        using TBackend = vtkStructuredTCellBackend<VTK_XY_PLANE, false>;
        backEnd = std::make_shared<TBackend>(dims);
        break;
      }
      case VTK_YZ_PLANE:
      {
        using TBackend = vtkStructuredTCellBackend<VTK_YZ_PLANE, false>;
        backEnd = std::make_shared<TBackend>(dims);
        break;
      }
      case VTK_XZ_PLANE:
      {
        using TBackend = vtkStructuredTCellBackend<VTK_XZ_PLANE, false>;
        backEnd = std::make_shared<TBackend>(dims);
        break;
      }
      case VTK_XYZ_GRID:
      {
        using TBackend = vtkStructuredTCellBackend<VTK_XYZ_GRID, false>;
        backEnd = std::make_shared<TBackend>(dims);
        break;
      }
      default:
      {
        vtkErrorMacro("Unsupported data description: " << description);
        return;
      }
    }
  }
  this->Connectivity->SetBackend(backEnd);
  this->Connectivity->SetNumberOfComponents(backEnd->GetCellSize());
  this->Connectivity->SetNumberOfTuples(vtkStructuredData::GetNumberOfCells(extent));
  this->Modified();
}

//------------------------------------------------------------------------------
vtkIdType vtkStructuredCellArray::IsHomogeneous()
{
  return this->Connectivity ? this->Connectivity->GetNumberOfComponents() : 0;
}

//------------------------------------------------------------------------------
void vtkStructuredCellArray::GetCellAtId(vtkIdType cellId, vtkIdType& cellSize,
  vtkIdType const*& cellPoints, vtkIdList* ptIds) VTK_SIZEHINT(cellPoints, cellSize)
{
  ptIds->SetNumberOfIds(this->Connectivity->GetNumberOfComponents());
  this->Connectivity->GetTypedTuple(cellId, ptIds->GetPointer(0));
  cellSize = this->Connectivity->GetNumberOfComponents();
  cellPoints = ptIds->GetPointer(0);
}

//------------------------------------------------------------------------------
void vtkStructuredCellArray::GetCellAtId(vtkIdType cellId, vtkIdList* ptIds)
{
  ptIds->SetNumberOfIds(this->Connectivity->GetNumberOfComponents());
  this->Connectivity->GetTypedTuple(cellId, ptIds->GetPointer(0));
}

//------------------------------------------------------------------------------
void vtkStructuredCellArray::GetCellAtId(
  vtkIdType cellId, vtkIdType& cellSize, vtkIdType* cellPoints) VTK_SIZEHINT(cellPoints, cellSize)
  VTK_EXPECTS(0 <= cellId && cellId < GetNumberOfCells())
{
  cellSize = this->Connectivity->GetNumberOfComponents();
  this->Connectivity->GetTypedTuple(cellId, cellPoints);
}

//------------------------------------------------------------------------------
void vtkStructuredCellArray::GetCellAtId(int ijk[3], vtkIdList* ptIds)
{
  ptIds->SetNumberOfIds(this->Connectivity->GetNumberOfComponents());
  this->Connectivity->GetBackend()->mapStructuredTuple(ijk, ptIds->GetPointer(0));
}

//------------------------------------------------------------------------------
void vtkStructuredCellArray::GetCellAtId(int ijk[3], vtkIdType& cellSize, vtkIdType* cellPoints)
{
  cellSize = this->Connectivity->GetNumberOfComponents();
  this->Connectivity->GetBackend()->mapStructuredTuple(ijk, cellPoints);
}

//------------------------------------------------------------------------------
vtkIdType vtkStructuredCellArray::GetCellSize(const vtkIdType vtkNotUsed(cellId)) const
{
  return this->Connectivity->GetNumberOfComponents();
}

//------------------------------------------------------------------------------
int vtkStructuredCellArray::GetMaxCellSize()
{
  return this->Connectivity->GetNumberOfComponents();
}

//------------------------------------------------------------------------------
void vtkStructuredCellArray::DeepCopy(vtkAbstractCellArray* ca)
{
  auto other = vtkStructuredCellArray::SafeDownCast(ca);
  if (!other)
  {
    vtkErrorMacro("Cannot copy from a different type of cell array.");
    return;
  }
  this->Connectivity = vtkSmartPointer<vtkImplicitArray<vtkStructuredCellBackend>>::New();
  this->Connectivity->ImplicitDeepCopy(other->Connectivity.Get());
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkStructuredCellArray::ShallowCopy(vtkAbstractCellArray* ca)
{
  auto other = vtkStructuredCellArray::SafeDownCast(ca);
  if (!other)
  {
    vtkErrorMacro("Cannot copy from a different type of cell array.");
    return;
  }
  this->Connectivity = other->Connectivity;
  this->Modified();
}
VTK_ABI_NAMESPACE_END
