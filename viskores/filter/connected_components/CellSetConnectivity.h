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

#ifndef viskores_filter_connected_components_CellSetConnectivity_h
#define viskores_filter_connected_components_CellSetConnectivity_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/connected_components/viskores_filter_connected_components_export.h>

namespace viskores
{
namespace filter
{
namespace connected_components
{

/// \brief Finds and labels groups of cells that are connected together through their topology.
///
/// Two cells are considered connected if they share an edge. `CellSetConnectivity` identifies some
/// number of components and assigns each component a unique integer.
///
/// The result of the filter is a cell field of type `viskores::Id` with the default name of
/// "component" (which can be changed with the `SetOutputFieldName` method). Each entry in
/// the cell field will be a number that identifies to which component the cell belongs.
class VISKORES_FILTER_CONNECTED_COMPONENTS_EXPORT CellSetConnectivity
  : public viskores::filter::Filter
{
public:
  VISKORES_CONT CellSetConnectivity() { this->SetOutputFieldName("component"); }

private:
  VISKORES_CONT
  viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;
};

} // namespace connected_components

} // namespace filter
} // namespace viskores

#endif //viskores_filter_connected_components_CellSetConnectivity_h
