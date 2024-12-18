// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkConduitArrayUtilitiesInternals
 * @brief helper to convert Conduit arrays to VTK arrays.
 * @ingroup Insitu
 *
 * vtkConduitArrayUtilitiesInternals is intended to convert Conduit nodes satisfying the
 * `mcarray` protocol to VTK arrays. It uses zero-copy, as much as possible.
 * Currently implementation fails if zero-copy is not possible. In future, that
 * may be changed to do a deep-copy (with appropriate warnings) if necessary.
 *
 * This is primarily designed for use by vtkConduitSource.
 */

#ifndef vtkConduitArrayUtilitiesInternals_h
#define vtkConduitArrayUtilitiesInternals_h

#include <catalyst_conduit.hpp> // for conduit_cpp::DataType::Id

namespace internals
{
VTK_ABI_NAMESPACE_BEGIN
conduit_cpp::DataType::Id GetTypeId(conduit_cpp::DataType::Id type, bool force_signed);
VTK_ABI_NAMESPACE_END
};

#endif
