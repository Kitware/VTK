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

#ifndef viskores_filter_flow_StreamSurface_h
#define viskores_filter_flow_StreamSurface_h

#include <viskores/filter/flow/Streamline.h>

namespace viskores
{
namespace filter
{
namespace flow
{

/// \brief Generate stream surfaces from a vector field.
///
/// This filter takes as input a velocity vector field and seed locations. The seed locations
/// should be arranged in a line or curve. The filter then traces the path each seed point
/// would take if moving at the velocity specified by the field and connects all the lines
/// together into a surface. Mathematically, this is the surface that is tangent to the
/// velocity field everywhere.
///
/// The output of this filter is a `viskores::cont::DataSet` containing a mesh for the created
/// surface.
class VISKORES_FILTER_FLOW_EXPORT StreamSurface : public viskores::filter::flow::Streamline
{
protected:
  VISKORES_CONT viskores::cont::PartitionedDataSet DoExecutePartitions(
    const viskores::cont::PartitionedDataSet& inData) override;
};

}
}
} // namespace viskores::filter::flow

#endif // viskores_filter_flow_StreamSurface_h
