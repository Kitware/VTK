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
#ifndef viskores_worklet_WorkletNeighborhood_h
#define viskores_worklet_WorkletNeighborhood_h

#include <viskores/worklet/internal/WorkletBase.h>

#include <viskores/TopologyElementTag.h>

#include <viskores/cont/arg/ControlSignatureTagBase.h>
#include <viskores/cont/arg/TransportTagArrayIn.h>
#include <viskores/cont/arg/TransportTagArrayInOut.h>
#include <viskores/cont/arg/TransportTagArrayOut.h>
#include <viskores/cont/arg/TransportTagCellSetIn.h>
#include <viskores/cont/arg/TypeCheckTagArrayIn.h>
#include <viskores/cont/arg/TypeCheckTagArrayInOut.h>
#include <viskores/cont/arg/TypeCheckTagArrayOut.h>
#include <viskores/cont/arg/TypeCheckTagCellSetStructured.h>

#include <viskores/exec/arg/Boundary.h>
#include <viskores/exec/arg/FetchTagArrayDirectIn.h>
#include <viskores/exec/arg/FetchTagArrayDirectInOut.h>
#include <viskores/exec/arg/FetchTagArrayDirectOut.h>
#include <viskores/exec/arg/FetchTagArrayNeighborhoodIn.h>
#include <viskores/exec/arg/FetchTagCellSetIn.h>

#include <viskores/worklet/BoundaryTypes.h>
#include <viskores/worklet/ScatterIdentity.h>

namespace viskores
{
namespace worklet
{

class WorkletNeighborhood : public viskores::worklet::internal::WorkletBase
{
public:
  /// @brief The `ExecutionSignature` tag to query if the current iteration is inside the boundary.
  ///
  /// This `ExecutionSignature` tag provides a `viskores::exec::BoundaryState` object that provides
  /// information about where the local neighborhood is in relationship to the full mesh. It allows
  /// you to query whether the neighborhood of the current worklet call is completely inside the
  /// bounds of the mesh or if it extends beyond the mesh. This is important as when you are on a
  /// boundary the neighboordhood will contain empty values for a certain subset of values, and in
  /// this case the values returned will depend on the boundary behavior.
  ///
  struct Boundary : viskores::exec::arg::Boundary
  {
  };

  /// All worklets must define their scatter operation.
  using ScatterType = viskores::worklet::ScatterIdentity;

  VISKORES_DEPRECATED_SUPPRESS_BEGIN
  /// All neighborhood worklets must define their boundary type operation.
  /// The boundary type determines how loading on boundaries will work.
  using BoundaryType VISKORES_DEPRECATED(2.2, "Never fully supported, so being removed.") =
    viskores::worklet::BoundaryClamp;

  /// In addition to defining the boundary type, the worklet must produce the
  /// boundary condition. The default BoundaryClamp has no state, so just return an
  /// instance.
  /// Note: Currently only BoundaryClamp is implemented
  VISKORES_DEPRECATED(2.2, "Never fully supported, so being removed.")
  VISKORES_CONT BoundaryType GetBoundaryCondition() const { return BoundaryType(); }
  VISKORES_DEPRECATED_SUPPRESS_END

  /// @brief A control signature tag for input fields.
  ///
  /// A `FieldIn` argument expects a `viskores::cont::ArrayHandle` in the associated
  /// parameter of the invoke. Each invocation of the worklet gets a single value
  /// out of this array.
  ///
  /// This tag means that the field is read only.
  ///
  struct FieldIn : viskores::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = viskores::cont::arg::TypeCheckTagArrayIn;
    using TransportTag = viskores::cont::arg::TransportTagArrayIn;
    using FetchTag = viskores::exec::arg::FetchTagArrayDirectIn;
  };

  /// @brief A control signature tag for output fields.
  ///
  /// A `FieldOut` argument expects a `viskores::cont::ArrayHandle` in the associated
  /// parameter of the invoke. The array is resized before scheduling begins, and
  /// each invocation of the worklet sets a single value in the array.
  ///
  /// This tag means that the field is write only.
  ///
  struct FieldOut : viskores::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = viskores::cont::arg::TypeCheckTagArrayOut;
    using TransportTag = viskores::cont::arg::TransportTagArrayOut;
    using FetchTag = viskores::exec::arg::FetchTagArrayDirectOut;
  };

  /// @brief A control signature tag for input-output (in-place) fields.
  ///
  /// A `FieldInOut` argument expects a `viskores::cont::ArrayHandle` in the
  /// associated parameter of the invoke. Each invocation of the worklet gets a
  /// single value out of this array, which is replaced by the resulting value
  /// after the worklet completes.
  ///
  /// This tag means that the field is read and write.
  ///
  struct FieldInOut : viskores::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = viskores::cont::arg::TypeCheckTagArrayInOut;
    using TransportTag = viskores::cont::arg::TransportTagArrayInOut;
    using FetchTag = viskores::exec::arg::FetchTagArrayDirectInOut;
  };

  /// @brief A control signature tag for input connectivity.
  ///
  /// This tag represents the cell set that defines the collection of points the
  /// map will operate on. A `CellSetIn` argument expects a `viskores::cont::CellSetStructured`
  /// object in the associated parameter of the invoke.
  ///
  /// There must be exactly one `CellSetIn` argument, and the worklet's `InputDomain` must
  /// be set to this argument.
  struct CellSetIn : viskores::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = viskores::cont::arg::TypeCheckTagCellSetStructured;
    using TransportTag =
      viskores::cont::arg::TransportTagCellSetIn<viskores::TopologyElementTagPoint,
                                                 viskores::TopologyElementTagCell>;
    using FetchTag = viskores::exec::arg::FetchTagCellSetIn;
  };

  /// @brief A control signature tag for neighborhood input values.
  ///
  /// A neighborhood worklet operates by allowing access to a adjacent element
  /// values in a NxNxN patch called a neighborhood.
  /// No matter the size of the neighborhood it is symmetric across its center
  /// in each axis, and the current point value will be at the center
  /// For example a 3x3x3 neighborhood would have local indices ranging from -1 to 1
  /// in each dimension.
  ///
  /// This tag specifies a `viskores::cont::ArrayHandle` object that holds the values. It is
  /// an input array with entries for each element.
  ///
  /// What differentiates `FieldInNeighborhood` from `FieldIn` is that `FieldInNeighborhood`
  /// allows the worklet function to access the field value at the element it is visiting and
  /// the field values in the neighborhood around it. Thus, instead of getting a single value
  /// out of the array, each invocation of the worklet gets a `viskores::exec::FieldNeighborhood`
  /// object. These objects allow retrieval of field values using indices relative to the
  /// visited element.
  ///
  struct FieldInNeighborhood : viskores::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = viskores::cont::arg::TypeCheckTagArrayIn;
    using TransportTag = viskores::cont::arg::TransportTagArrayIn;
    using FetchTag = viskores::exec::arg::FetchTagArrayNeighborhoodIn;
  };
};
} // namespace worklet
} // namespace viskores

#endif // viskores_worklet_WorkletPointNeighborhood_h
