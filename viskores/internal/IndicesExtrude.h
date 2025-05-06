//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_internal_IndicesExtrude_h
#define viskores_internal_IndicesExtrude_h

#include <viskores/Math.h>
#include <viskores/Types.h>

namespace viskores
{
namespace exec
{

struct IndicesExtrude
{
  IndicesExtrude() = default;

  VISKORES_EXEC
  IndicesExtrude(viskores::Vec3i_32 pointIds1,
                 viskores::Int32 plane1,
                 viskores::Vec3i_32 pointIds2,
                 viskores::Int32 plane2,
                 viskores::Int32 numberOfPointsPerPlane)
    : PointIds{ pointIds1, pointIds2 }
    , Planes{ plane1, plane2 }
    , NumberOfPointsPerPlane(numberOfPointsPerPlane)
  {
  }

  VISKORES_EXEC
  viskores::Id operator[](viskores::IdComponent index) const
  {
    VISKORES_ASSERT(index >= 0 && index < 6);
    if (index < 3)
    {
      return (static_cast<viskores::Id>(this->NumberOfPointsPerPlane) * this->Planes[0]) +
        this->PointIds[0][index];
    }
    else
    {
      return (static_cast<viskores::Id>(this->NumberOfPointsPerPlane) * this->Planes[1]) +
        this->PointIds[1][index - 3];
    }
  }

  VISKORES_EXEC
  constexpr viskores::IdComponent GetNumberOfComponents() const { return 6; }

  template <typename T, viskores::IdComponent DestSize>
  VISKORES_EXEC void CopyInto(viskores::Vec<T, DestSize>& dest) const
  {
    for (viskores::IdComponent i = 0; i < viskores::Min(6, DestSize); ++i)
    {
      dest[i] = (*this)[i];
    }
  }

  viskores::Vec3i_32 PointIds[2];
  viskores::Int32 Planes[2];
  viskores::Int32 NumberOfPointsPerPlane;
};

template <typename ConnectivityPortalType>
struct ReverseIndicesExtrude
{
  ReverseIndicesExtrude() = default;

  VISKORES_EXEC
  ReverseIndicesExtrude(const ConnectivityPortalType conn,
                        viskores::Id offset1,
                        viskores::IdComponent length1,
                        viskores::Id offset2,
                        viskores::IdComponent length2,
                        viskores::IdComponent plane1,
                        viskores::IdComponent plane2,
                        viskores::Int32 numberOfCellsPerPlane)
    : Connectivity(conn)
    , Offset1(offset1)
    , Offset2(offset2)
    , Length1(length1)
    , NumberOfComponents(length1 + length2)
    , CellOffset1(plane1 * numberOfCellsPerPlane)
    , CellOffset2(plane2 * numberOfCellsPerPlane)
  {
  }

  VISKORES_EXEC
  viskores::Id operator[](viskores::IdComponent index) const
  {
    VISKORES_ASSERT(index >= 0 && index < (this->NumberOfComponents));
    if (index < this->Length1)
    {
      return this->Connectivity.Get(this->Offset1 + index) + this->CellOffset1;
    }
    else
    {
      return this->Connectivity.Get(this->Offset2 + index - this->Length1) + this->CellOffset2;
    }
  }

  VISKORES_EXEC
  viskores::IdComponent GetNumberOfComponents() const { return this->NumberOfComponents; }

  template <typename T, viskores::IdComponent DestSize>
  VISKORES_EXEC void CopyInto(viskores::Vec<T, DestSize>& dest) const
  {
    for (viskores::IdComponent i = 0; i < viskores::Min(this->NumberOfComponents, DestSize); ++i)
    {
      dest[i] = (*this)[i];
    }
  }

  ConnectivityPortalType Connectivity;
  viskores::Id Offset1, Offset2;
  viskores::IdComponent Length1;
  viskores::IdComponent NumberOfComponents;
  viskores::Id CellOffset1, CellOffset2;
};
}
}

#endif //viskores_m_internal_IndicesExtrude_h
