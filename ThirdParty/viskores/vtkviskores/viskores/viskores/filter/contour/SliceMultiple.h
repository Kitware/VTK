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
#ifndef viskores_filter_contour_Slice_Multi_h
#define viskores_filter_contour_Slice_Multi_h

#include <viskores/filter/contour/Contour.h>
#include <viskores/filter/contour/viskores_filter_contour_export.h>

#include <viskores/ImplicitFunction.h>

namespace viskores
{
namespace filter
{
namespace contour
{
/// \brief This filter can accept multiple implicit functions used by the slice filter.
/// It returns a merged data set that contains multiple results returned by the slice filter.
class VISKORES_FILTER_CONTOUR_EXPORT SliceMultiple : public viskores::filter::contour::Contour
{
public:
  /// Set/Get the implicit function that is used to perform the slicing.
  ///
  VISKORES_CONT
  void AddImplicitFunction(const viskores::ImplicitFunctionGeneral& func)
  {
    FunctionList.push_back(func);
  }
  VISKORES_CONT
  const viskores::ImplicitFunctionGeneral& GetImplicitFunction(viskores::Id id) const
  {
    VISKORES_ASSERT(id < static_cast<viskores::Id>(FunctionList.size()));
    return FunctionList[id];
  }

private:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;

  std::vector<viskores::ImplicitFunctionGeneral> FunctionList;
};
} // namespace contour
} // namespace filter
} // namespace viskores

#endif // viskores_filter_contour_Slice_Multi_h
