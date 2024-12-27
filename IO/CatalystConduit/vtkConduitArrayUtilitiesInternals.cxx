// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkConduitArrayUtilitiesInternals.h"

#include <catalyst_conduit.hpp>

namespace internals
{
VTK_ABI_NAMESPACE_BEGIN

//----------------------------------------------------------------------------
conduit_cpp::DataType::Id GetTypeId(conduit_cpp::DataType::Id type, bool force_signed)
{
  if (!force_signed)
  {
    return type;
  }
  switch (type)
  {
    case conduit_cpp::DataType::Id::uint8:
      return conduit_cpp::DataType::Id::int8;

    case conduit_cpp::DataType::Id::uint16:
      return conduit_cpp::DataType::Id::int16;

    case conduit_cpp::DataType::Id::uint32:
      return conduit_cpp::DataType::Id::int32;

    case conduit_cpp::DataType::Id::uint64:
      return conduit_cpp::DataType::Id::int64;

    default:
      return type;
  }
}

VTK_ABI_NAMESPACE_END
} // internals
