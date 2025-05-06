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

#ifndef viskores_filter_FilterField_h
#define viskores_filter_FilterField_h

#include <viskores/filter/Filter.h>

#include <viskores/Deprecated.h>


namespace viskores
{
namespace filter
{

struct VISKORES_DEPRECATED(
  2.2,
  "FilterField.h (and its class) are deprecated. Use Filter.h (and its class).")
  viskores_filter_FilterField_h_deprecated
{
};

static viskores_filter_FilterField_h_deprecated issue_deprecated_warning_filterfield()
{
  viskores_filter_FilterField_h_deprecated x;
  return x;
}

} // namespace filter
} // namespace viskores

#endif // viskores_filter_FilterField_h
