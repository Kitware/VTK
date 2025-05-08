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
#ifndef viskores_cont_CellLocatorGeneral_h
#define viskores_cont_CellLocatorGeneral_h

#include <viskores/cont/CellLocatorRectilinearGrid.h>
#include <viskores/cont/CellLocatorTwoLevel.h>
#include <viskores/cont/CellLocatorUniformGrid.h>

#include <viskores/exec/CellLocatorMultiplexer.h>

#include <viskores/cont/Variant.h>

#include <functional>
#include <memory>

namespace viskores
{
namespace cont
{

/// \brief A CellLocator that works generally well for any supported cell set.
///
/// `CellLocatorGeneral` creates a `CellLocator` that acts like a multiplexer to
/// switch at runtime to any supported cell set. It is a convenient class to use
/// when the type of `CellSet` cannot be determined at runtime.
///
/// Note that `CellLocatorGeneral` only supports a finite amount of `CellSet` types.
/// Thus, it is possible to give it a cell set type that is not supported.
///
/// Also note that `CellLocatorGeneral` can add a significant amount of code inside
/// of worklet that uses it, and this might cause some issues with some compilers.
///
class VISKORES_CONT_EXPORT CellLocatorGeneral : public viskores::cont::CellLocatorBase
{
public:
  using ContLocatorList = viskores::List<viskores::cont::CellLocatorUniformGrid,
                                         viskores::cont::CellLocatorRectilinearGrid,
                                         viskores::cont::CellLocatorTwoLevel>;

  using ExecLocatorList = viskores::List<
    viskores::cont::internal::ExecutionObjectType<viskores::cont::CellLocatorUniformGrid>,
    viskores::cont::internal::ExecutionObjectType<viskores::cont::CellLocatorRectilinearGrid>,
    viskores::cont::internal::ExecutionObjectType<viskores::cont::CellLocatorTwoLevel>>;

  using ExecObjType = viskores::ListApply<ExecLocatorList, viskores::exec::CellLocatorMultiplexer>;
  using LastCell = typename ExecObjType::LastCell;

  VISKORES_CONT ExecObjType PrepareForExecution(viskores::cont::DeviceAdapterId device,
                                                viskores::cont::Token& token) const;

private:
  viskores::cont::ListAsVariant<ContLocatorList> LocatorImpl;

  VISKORES_CONT void Build() override;

  struct PrepareFunctor;
};
}
} // viskores::cont

#endif // viskores_cont_CellLocatorGeneral_h
