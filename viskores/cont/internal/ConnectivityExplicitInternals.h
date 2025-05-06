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
#ifndef viskores_cont_internal_ConnectivityExplicitInternals_h
#define viskores_cont_internal_ConnectivityExplicitInternals_h

#include <viskores/CellShape.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleCast.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayHandleCounting.h>

namespace viskores
{
namespace cont
{
namespace internal
{

template <typename ShapesStorageTag = VISKORES_DEFAULT_STORAGE_TAG,
          typename ConnectivityStorageTag = VISKORES_DEFAULT_STORAGE_TAG,
          typename OffsetsStorageTag = VISKORES_DEFAULT_STORAGE_TAG>
struct ConnectivityExplicitInternals
{
  using ShapesArrayType = viskores::cont::ArrayHandle<viskores::UInt8, ShapesStorageTag>;
  using ConnectivityArrayType = viskores::cont::ArrayHandle<viskores::Id, ConnectivityStorageTag>;
  using OffsetsArrayType = viskores::cont::ArrayHandle<viskores::Id, OffsetsStorageTag>;

  ShapesArrayType Shapes;
  ConnectivityArrayType Connectivity;
  OffsetsArrayType Offsets;

  bool ElementsValid;

  VISKORES_CONT
  ConnectivityExplicitInternals()
    : ElementsValid(false)
  {
  }

  VISKORES_CONT
  viskores::Id GetNumberOfElements() const
  {
    VISKORES_ASSERT(this->ElementsValid);

    return this->Shapes.GetNumberOfValues();
  }

  VISKORES_CONT
  void ReleaseResourcesExecution()
  {
    this->Shapes.ReleaseResourcesExecution();
    this->Connectivity.ReleaseResourcesExecution();
    this->Offsets.ReleaseResourcesExecution();
  }

  VISKORES_CONT
  void PrintSummary(std::ostream& out) const
  {
    if (this->ElementsValid)
    {
      out << "     Shapes: ";
      viskores::cont::printSummary_ArrayHandle(this->Shapes, out);
      out << "     Connectivity: ";
      viskores::cont::printSummary_ArrayHandle(this->Connectivity, out);
      out << "     Offsets: ";
      viskores::cont::printSummary_ArrayHandle(this->Offsets, out);
    }
    else
    {
      out << "     Not Allocated" << std::endl;
    }
  }
};
}
}
} // namespace viskores::cont::internal

#endif //viskores_cont_internal_ConnectivityExplicitInternals_h
