// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkStructuredPointBackend_txx
#define vtkStructuredPointBackend_txx

#include "vtkStructuredPointBackend.h"

VTK_ABI_NAMESPACE_BEGIN

//----------------------------------------------------------------------------
template <typename ValueType>
vtkStructuredPointBackend<ValueType>::vtkStructuredPointBackend() = default;

//----------------------------------------------------------------------------
template <typename ValueType>
vtkStructuredPointBackend<ValueType>::~vtkStructuredPointBackend() = default;

//----------------------------------------------------------------------------
template <typename ValueType, typename ArrayTypeX, typename ArrayTypeY, typename ArrayTypeZ,
  int DataDescription, bool UseDirMatrix>
vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ, DataDescription,
  UseDirMatrix>::vtkStructuredTPointBackend() = default;

//----------------------------------------------------------------------------
template <typename ValueType, typename ArrayTypeX, typename ArrayTypeY, typename ArrayTypeZ,
  int DataDescription, bool UseDirMatrix>
vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ, DataDescription,
  UseDirMatrix>::~vtkStructuredTPointBackend() = default;

//----------------------------------------------------------------------------
template <typename ValueType, typename ArrayTypeX, typename ArrayTypeY, typename ArrayTypeZ,
  int DataDescription, bool UseDirMatrix>
void vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ, DataDescription,
  UseDirMatrix>::TransformIndexToPhysicalPoint(int i, int j, int k, ValueType* out) const
{
  auto mdata = this->IndexToPhysicalMatrix.data();
  out[0] = mdata[0] * i + mdata[1] * j + mdata[2] * k + mdata[3];
  out[1] = mdata[4] * i + mdata[5] * j + mdata[6] * k + mdata[7];
  out[2] = mdata[8] * i + mdata[9] * j + mdata[10] * k + mdata[11];
}

//----------------------------------------------------------------------------
template <typename ValueType, typename ArrayTypeX, typename ArrayTypeY, typename ArrayTypeZ,
  int DataDescription, bool UseDirMatrix>
template <int Description>
typename std::enable_if<(Description == 9 /*VTK_EMPTY*/), ValueType>::type
vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ, DataDescription,
  UseDirMatrix>::mapStructuredXComponentImpl(int vtkNotUsed(i)) const
{
  return 0.0;
}

//----------------------------------------------------------------------------
template <typename ValueType, typename ArrayTypeX, typename ArrayTypeY, typename ArrayTypeZ,
  int DataDescription, bool UseDirMatrix>
template <int Description>
typename std::enable_if<(Description != 9 /*VTK_EMPTY*/), ValueType>::type
vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ, DataDescription,
  UseDirMatrix>::mapStructuredXComponentImpl(int i) const
{
  return this->X[i];
}

//----------------------------------------------------------------------------
template <typename ValueType, typename ArrayTypeX, typename ArrayTypeY, typename ArrayTypeZ,
  int DataDescription, bool UseDirMatrix>
ValueType vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ, DataDescription,
  UseDirMatrix>::mapStructuredXComponent(int i) const
{
  return this->mapStructuredXComponentImpl(i);
}

//----------------------------------------------------------------------------
template <typename ValueType, typename ArrayTypeX, typename ArrayTypeY, typename ArrayTypeZ,
  int DataDescription, bool UseDirMatrix>
template <int Description>
typename std::enable_if<(Description == 9 /*VTK_EMPTY*/), ValueType>::type
vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ, DataDescription,
  UseDirMatrix>::mapStructuredYComponentImpl(int vtkNotUsed(j)) const
{
  return 0.0;
}

//----------------------------------------------------------------------------
template <typename ValueType, typename ArrayTypeX, typename ArrayTypeY, typename ArrayTypeZ,
  int DataDescription, bool UseDirMatrix>
template <int Description>
typename std::enable_if<(Description != 9 /*VTK_EMPTY*/), ValueType>::type
vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ, DataDescription,
  UseDirMatrix>::mapStructuredYComponentImpl(int j) const
{
  return this->Y[j];
}

//----------------------------------------------------------------------------
template <typename ValueType, typename ArrayTypeX, typename ArrayTypeY, typename ArrayTypeZ,
  int DataDescription, bool UseDirMatrix>
ValueType vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ, DataDescription,
  UseDirMatrix>::mapStructuredYComponent(int j) const
{
  return this->mapStructuredYComponentImpl(j);
}

//----------------------------------------------------------------------------
template <typename ValueType, typename ArrayTypeX, typename ArrayTypeY, typename ArrayTypeZ,
  int DataDescription, bool UseDirMatrix>
template <int Description>
typename std::enable_if<(Description == 9 /*VTK_EMPTY*/), ValueType>::type
vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ, DataDescription,
  UseDirMatrix>::mapStructuredZComponentImpl(int vtkNotUsed(k)) const
{
  return 0.0;
}

//----------------------------------------------------------------------------
template <typename ValueType, typename ArrayTypeX, typename ArrayTypeY, typename ArrayTypeZ,
  int DataDescription, bool UseDirMatrix>
template <int Description>
typename std::enable_if<(Description != 9 /*VTK_EMPTY*/), ValueType>::type
vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ, DataDescription,
  UseDirMatrix>::mapStructuredZComponentImpl(int k) const
{
  return this->Z[k];
}

//----------------------------------------------------------------------------
template <typename ValueType, typename ArrayTypeX, typename ArrayTypeY, typename ArrayTypeZ,
  int DataDescription, bool UseDirMatrix>
ValueType vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ, DataDescription,
  UseDirMatrix>::mapStructuredZComponent(int k) const
{
  return this->mapStructuredZComponentImpl(k);
}

//----------------------------------------------------------------------------
template <typename ValueType, typename ArrayTypeX, typename ArrayTypeY, typename ArrayTypeZ,
  int DataDescription, bool UseDirMatrix>
template <int Description>
typename std::enable_if<(Description == 9 /*VTK_EMPTY*/), void>::type
vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ, DataDescription,
  UseDirMatrix>::mapStructuredTupleImpl(int vtkNotUsed(ijk)[3], ValueType* tuple) const
{
  tuple[0] = 0.0;
  tuple[1] = 0.0;
  tuple[2] = 0.0;
}

//----------------------------------------------------------------------------
template <typename ValueType, typename ArrayTypeX, typename ArrayTypeY, typename ArrayTypeZ,
  int DataDescription, bool UseDirMatrix>
template <int Description>
typename std::enable_if<(Description != 9 /*VTK_EMPTY*/), void>::type
vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ, DataDescription,
  UseDirMatrix>::mapStructuredTupleImpl(int ijk[3], ValueType* tuple) const
{
  tuple[0] = this->X[ijk[0]];
  tuple[1] = this->Y[ijk[1]];
  tuple[2] = this->Z[ijk[2]];
}

//----------------------------------------------------------------------------
template <typename ValueType, typename ArrayTypeX, typename ArrayTypeY, typename ArrayTypeZ,
  int DataDescription, bool UseDirMatrix>
void vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ, DataDescription,
  UseDirMatrix>::mapStructuredTuple(int ijk[3], ValueType tuple[3]) const
{
  if (!UseDirMatrix)
  {
    this->mapStructuredTupleImpl(ijk, tuple);
  }
  else
  {
    this->TransformIndexToPhysicalPoint(
      ijk[0] + this->Extent[0], ijk[1] + this->Extent[2], ijk[2] + this->Extent[4], tuple);
  }
}

//----------------------------------------------------------------------------
template <typename ValueType, typename ArrayTypeX, typename ArrayTypeY, typename ArrayTypeZ,
  int DataDescription, bool UseDirMatrix>
template <int Description>
typename std::enable_if<(Description == 9 /*VTK_EMPTY*/), void>::type
vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ, DataDescription,
  UseDirMatrix>::ComputePointStructuredCoords(vtkIdType vtkNotUsed(pointId), int ijk[3]) const
{
  ijk[0] = ijk[1] = ijk[2] = 0;
}

//----------------------------------------------------------------------------
template <typename ValueType, typename ArrayTypeX, typename ArrayTypeY, typename ArrayTypeZ,
  int DataDescription, bool UseDirMatrix>
template <int Description>
typename std::enable_if<(Description == 1 /*VTK_SINGLE_POINT*/), void>::type
vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ, DataDescription,
  UseDirMatrix>::ComputePointStructuredCoords(vtkIdType vtkNotUsed(pointId), int ijk[3]) const
{
  ijk[0] = ijk[1] = ijk[2] = 0;
}

//----------------------------------------------------------------------------
template <typename ValueType, typename ArrayTypeX, typename ArrayTypeY, typename ArrayTypeZ,
  int DataDescription, bool UseDirMatrix>
template <int Description>
typename std::enable_if<(Description == 2 /*VTK_X_LINE*/), void>::type
vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ, DataDescription,
  UseDirMatrix>::ComputePointStructuredCoords(vtkIdType pointId, int ijk[3]) const
{
  ijk[0] = pointId;
  ijk[1] = 0;
  ijk[2] = 0;
}

//----------------------------------------------------------------------------
template <typename ValueType, typename ArrayTypeX, typename ArrayTypeY, typename ArrayTypeZ,
  int DataDescription, bool UseDirMatrix>
template <int Description>
typename std::enable_if<(Description == 3 /*VTK_Y_LINE*/), void>::type
vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ, DataDescription,
  UseDirMatrix>::ComputePointStructuredCoords(vtkIdType pointId, int ijk[3]) const
{
  ijk[0] = 0;
  ijk[1] = pointId;
  ijk[2] = 0;
}

//----------------------------------------------------------------------------
template <typename ValueType, typename ArrayTypeX, typename ArrayTypeY, typename ArrayTypeZ,
  int DataDescription, bool UseDirMatrix>
template <int Description>
typename std::enable_if<(Description == 4 /*VTK_Z_LINE*/), void>::type
vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ, DataDescription,
  UseDirMatrix>::ComputePointStructuredCoords(vtkIdType pointId, int ijk[3]) const
{
  ijk[0] = 0;
  ijk[1] = 0;
  ijk[2] = pointId;
}

//----------------------------------------------------------------------------
template <typename ValueType, typename ArrayTypeX, typename ArrayTypeY, typename ArrayTypeZ,
  int DataDescription, bool UseDirMatrix>
template <int Description>
typename std::enable_if<(Description == 5 /*VTK_XY_PLANE*/), void>::type
vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ, DataDescription,
  UseDirMatrix>::ComputePointStructuredCoords(vtkIdType pointId, int ijk[3]) const
{
  const auto div = std::div(pointId, this->Dimensions[0]);
  ijk[0] = div.rem;
  ijk[1] = div.quot;
  ijk[2] = 0;
}

//----------------------------------------------------------------------------
template <typename ValueType, typename ArrayTypeX, typename ArrayTypeY, typename ArrayTypeZ,
  int DataDescription, bool UseDirMatrix>
template <int Description>
typename std::enable_if<(Description == 6 /*VTK_YZ_PLANE*/), void>::type
vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ, DataDescription,
  UseDirMatrix>::ComputePointStructuredCoords(vtkIdType pointId, int ijk[3]) const
{
  const auto div = std::div(pointId, this->Dimensions[1]);
  ijk[0] = 0;
  ijk[1] = div.rem;
  ijk[2] = div.quot;
}

//----------------------------------------------------------------------------
template <typename ValueType, typename ArrayTypeX, typename ArrayTypeY, typename ArrayTypeZ,
  int DataDescription, bool UseDirMatrix>
template <int Description>
typename std::enable_if<(Description == 7 /*VTK_XZ_PLANE*/), void>::type
vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ, DataDescription,
  UseDirMatrix>::ComputePointStructuredCoords(vtkIdType pointId, int ijk[3]) const
{
  const auto div = std::div(pointId, this->Dimensions[0]);
  ijk[0] = div.rem;
  ijk[1] = 0;
  ijk[2] = div.quot;
}

//----------------------------------------------------------------------------
template <typename ValueType, typename ArrayTypeX, typename ArrayTypeY, typename ArrayTypeZ,
  int DataDescription, bool UseDirMatrix>
template <int Description>
typename std::enable_if<(Description == 8 /*VTK_XYZ_GRID*/), void>::type
vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ, DataDescription,
  UseDirMatrix>::ComputePointStructuredCoords(vtkIdType pointId, int ijk[3]) const
{
  const auto div1 = std::div(pointId, this->Dimensions[0]);
  const auto div2 = std::div(div1.quot, this->Dimensions[1]);
  ijk[0] = div1.rem;
  ijk[1] = div2.rem;
  ijk[2] = div2.quot;
}

//----------------------------------------------------------------------------
template <typename ValueType, typename ArrayTypeX, typename ArrayTypeY, typename ArrayTypeZ,
  int DataDescription, bool UseDirMatrix>
void vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ, DataDescription,
  UseDirMatrix>::mapTuple(vtkIdType tupleId, ValueType tuple[3]) const
{
  int ijk[3];
  this->ComputePointStructuredCoords(tupleId, ijk);
  if (!UseDirMatrix)
  {
    tuple[0] = this->X[ijk[0]];
    tuple[1] = this->Y[ijk[1]];
    tuple[2] = this->Z[ijk[2]];
  }
  else
  {
    this->TransformIndexToPhysicalPoint(
      ijk[0] + this->Extent[0], ijk[1] + this->Extent[2], ijk[2] + this->Extent[4], tuple);
  }
}

//----------------------------------------------------------------------------
template <typename ValueType, typename ArrayTypeX, typename ArrayTypeY, typename ArrayTypeZ,
  int DataDescription, bool UseDirMatrix>
template <int Description>
typename std::enable_if<(Description == 9 /*VTK_EMPTY*/), ValueType>::type
vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ, DataDescription,
  UseDirMatrix>::mapComponentImpl(vtkIdType vtkNotUsed(tupleId), int vtkNotUsed(comp)) const
{
  return 0.0;
}

//----------------------------------------------------------------------------
template <typename ValueType, typename ArrayTypeX, typename ArrayTypeY, typename ArrayTypeZ,
  int DataDescription, bool UseDirMatrix>
template <int Description>
typename std::enable_if<(Description == 1 /*VTK_SINGLE_POINT*/), ValueType>::type
vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ, DataDescription,
  UseDirMatrix>::mapComponentImpl(vtkIdType vtkNotUsed(tupleId), int comp) const
{
  switch (comp)
  {
    case 0:
      return this->X[0];
    case 1:
      return this->Y[0];
    case 2:
      return this->Z[0];
    default:
      return 0.0;
  }
}

//----------------------------------------------------------------------------
template <typename ValueType, typename ArrayTypeX, typename ArrayTypeY, typename ArrayTypeZ,
  int DataDescription, bool UseDirMatrix>
template <int Description>
typename std::enable_if<(Description == 2 /*VTK_X_LINE*/), ValueType>::type
vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ, DataDescription,
  UseDirMatrix>::mapComponentImpl(vtkIdType tupleId, int comp) const
{
  switch (comp)
  {
    case 0:
      return this->X[tupleId];
    case 1:
      return this->Y[0];
    case 2:
      return this->Z[0];
    default:
      return 0.0;
  }
}

//----------------------------------------------------------------------------
template <typename ValueType, typename ArrayTypeX, typename ArrayTypeY, typename ArrayTypeZ,
  int DataDescription, bool UseDirMatrix>
template <int Description>
typename std::enable_if<(Description == 3 /*VTK_Y_LINE*/), ValueType>::type
vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ, DataDescription,
  UseDirMatrix>::mapComponentImpl(vtkIdType tupleId, int comp) const
{
  switch (comp)
  {
    case 0:
      return this->X[0];
    case 1:
      return this->Y[tupleId];
    case 2:
      return this->Z[0];
    default:
      return 0.0;
  }
}

//----------------------------------------------------------------------------
template <typename ValueType, typename ArrayTypeX, typename ArrayTypeY, typename ArrayTypeZ,
  int DataDescription, bool UseDirMatrix>
template <int Description>
typename std::enable_if<(Description == 4 /*VTK_Z_LINE*/), ValueType>::type
vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ, DataDescription,
  UseDirMatrix>::mapComponentImpl(vtkIdType tupleId, int comp) const
{
  switch (comp)
  {
    case 0:
      return this->X[0];
    case 1:
      return this->Y[0];
    case 2:
      return this->Z[tupleId];
    default:
      return 0.0;
  }
}

//----------------------------------------------------------------------------
template <typename ValueType, typename ArrayTypeX, typename ArrayTypeY, typename ArrayTypeZ,
  int DataDescription, bool UseDirMatrix>
template <int Description>
typename std::enable_if<(Description == 5 /*VTK_XY_PLANE*/), ValueType>::type
vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ, DataDescription,
  UseDirMatrix>::mapComponentImpl(vtkIdType tupleId, int comp) const
{
  switch (comp)
  {
    case 0:
      return this->X[tupleId % this->Dimensions[0]];
    case 1:
      return this->Y[tupleId / this->Dimensions[0]];
    case 2:
      return this->Z[0];
    default:
      return 0.0;
  }
}

//----------------------------------------------------------------------------
template <typename ValueType, typename ArrayTypeX, typename ArrayTypeY, typename ArrayTypeZ,
  int DataDescription, bool UseDirMatrix>
template <int Description>
typename std::enable_if<(Description == 6 /*VTK_YZ_PLANE*/), ValueType>::type
vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ, DataDescription,
  UseDirMatrix>::mapComponentImpl(vtkIdType tupleId, int comp) const
{
  switch (comp)
  {
    case 0:
      return this->X[0];
    case 1:
      return this->Y[tupleId % this->Dimensions[1]];
    case 2:
      return this->Z[tupleId / this->Dimensions[1]];
    default:
      return 0.0;
  }
}

//----------------------------------------------------------------------------
template <typename ValueType, typename ArrayTypeX, typename ArrayTypeY, typename ArrayTypeZ,
  int DataDescription, bool UseDirMatrix>
template <int Description>
typename std::enable_if<(Description == 7 /*VTK_XZ_PLANE*/), ValueType>::type
vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ, DataDescription,
  UseDirMatrix>::mapComponentImpl(vtkIdType tupleId, int comp) const
{
  switch (comp)
  {
    case 0:
      return this->X[tupleId % this->Dimensions[0]];
    case 1:
      return this->Y[0];
    case 2:
      return this->Z[tupleId / this->Dimensions[0]];
    default:
      return 0.0;
  }
}

//----------------------------------------------------------------------------
template <typename ValueType, typename ArrayTypeX, typename ArrayTypeY, typename ArrayTypeZ,
  int DataDescription, bool UseDirMatrix>
template <int Description>
typename std::enable_if<(Description == 8 /*VTK_XYZ_GRID*/), ValueType>::type
vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ, DataDescription,
  UseDirMatrix>::mapComponentImpl(vtkIdType tupleId, int comp) const
{
  switch (comp)
  {
    case 0:
      return this->X[tupleId % this->Dimensions[0]];
    case 1:
      return this->Y[(tupleId / this->Dimensions[0]) % this->Dimensions[1]];
    case 2:
      return this->Z[tupleId / (this->Dimension_0_BY_1)];
    default:
      return 0.0;
  }
}

//----------------------------------------------------------------------------
template <typename ValueType, typename ArrayTypeX, typename ArrayTypeY, typename ArrayTypeZ,
  int DataDescription, bool UseDirMatrix>
ValueType vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ, DataDescription,
  UseDirMatrix>::mapComponent(vtkIdType tupleId, int comp) const
{
  if (!UseDirMatrix)
  {
    return this->mapComponentImpl(tupleId, comp);
  }
  else
  {
    ValueType tuple[3];
    this->mapTuple(tupleId, tuple);
    return tuple[comp];
  }
}

//----------------------------------------------------------------------------
template <typename ValueType, typename ArrayTypeX, typename ArrayTypeY, typename ArrayTypeZ,
  int DataDescription, bool UseDirMatrix>
ValueType vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ, DataDescription,
  UseDirMatrix>::map(vtkIdType valueId) const
{
  static constexpr vtkIdType denominator = 3;
  const auto div = std::div(valueId, denominator);
  return this->mapComponent(div.quot, div.rem);
}
VTK_ABI_NAMESPACE_END

#endif // vtkStructuredPointBackend_txx
