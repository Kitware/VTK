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

#ifndef viskores_source_Source_h
#define viskores_source_Source_h

#include <viskores/cont/DataSet.h>
#include <viskores/cont/Invoker.h>
#include <viskores/source/viskores_source_export.h>

namespace viskores
{
namespace source
{

/// \brief Base class for sources that generate a single data set.
///
/// A source creates a new \c viskores::cont::DataSet when \c Execute is
/// called. Subclasses implement \c DoExecute to construct the concrete data
/// set.
class VISKORES_SOURCE_EXPORT Source
{
public:
  VISKORES_CONT
  virtual ~Source() = default;

  /// \brief Generates the data set for this source.
  ///
  /// The returned data set is newly generated from the current source
  /// parameters.
  viskores::cont::DataSet Execute() const { return this->DoExecute(); }

protected:
  /// \brief Implementation hook used by subclasses to generate their data.
  virtual viskores::cont::DataSet DoExecute() const = 0;

  /// \brief Invoker used by source implementations to run worklets.
  viskores::cont::Invoker Invoke;
};

} // namespace source
} // namespace viskores

#endif // viskores_source_Source_h
